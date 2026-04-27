#include <stdio.h>
#include <cstdlib>
#include <string.h>
#include "rtmidi_c.h"

#include "miniaudio.h"

#define UNIX
#include "common.h"

struct Timer
{
    bool active;
    f32 time_left;
};

void TimerStart(Timer *timer, f32 time)
{
    timer->active = true;
    timer->time_left = time;
}

bool TimerUpdate(Timer *timer, f32 delta)
{
    if (!timer->active)
    {
	return false;
    }

    timer->time_left -= delta;

    if (timer->time_left <= 0)
    {
	*timer = {};
	retrurn true;
    }

    return false;
}

struct State
{
    u32 sequence_index;
    Timer reset_timer;
};

State state;

u32 key_sequence[] = {
    53,		// 11
    64,
    64,
    67,
    53,
    50,		// 9
    55,		// 12
    48		// 8
};

u32 octave_offsets[] = {
    2,                  // c to d
    2,                  // d to e
    1,                  // e
    2,                  // f
    2,                  // g
    2,                  // a
    1,                  // h
};

void SelectMidiDevice(RtMidiInPtr &midiin)
{
    static bool connected = false;

    if (connected)
    {
        return;
    }

    char device_name[128];
    i32 device_index = -1;

    u32 port_count = rtmidi_get_port_count(midiin);

    for (u32 i = 0; i < port_count; ++i)
    {
        i32 device_name_length = sizeof(device_name);
        rtmidi_get_port_name(midiin, i, device_name, &device_name_length);

        printf("%s\n", device_name);

        if (strcmp("QX49", device_name) <= 0)
        {
            rtmidi_open_port(midiin, i, device_name);
            connected = true;
            return;
        }
    }

    printf("Failed to find keyboard\n");
}

int main()
{
    SleepMilliseconds(3000);
    printf("hello world\n");

    InitializeModbus();

    //
    // Audio
    //

    ma_engine audio;

    if (ma_engine_init(NULL, &audio) != MA_SUCCESS)
    {
        printf("Failed to initialize miniaudio\n");
        return -1;
    }

    ma_sound keyboard_sounds[29];
    memset(keyboard_sounds, 0, sizeof(keyboard_sounds));
    for (u32 i = 0; i < lengthof(keyboard_sounds); ++i)
    {
	u32 id = i + 1;
        const char *fmt = "/home/escaperoom/sounds/t%u.wav"; 

        if (id < 10)
        {
            fmt = "/home/escaperoom/sounds/t0%u.wav";
        }

        char buffer[128] = {};
        sprintf(buffer, fmt, id);

        u32 flags = 0x00004002;
        ma_sound_init_from_file(&audio, buffer, flags, NULL, NULL, keyboard_sounds + i);
    }

    ma_sound *current_sound = NULL;

#if 0
    // This plays continously in the background to stop the hdmi audio connection from resetting.
    ma_sound keep_alive_sound;
    ma_sound_init_from_file(&audio, "assets/A_HWTH.wav", 0x00004002, NULL, NULL, &keep_alive_sound);
    ma_sound_set_volume(&keep_alive_sound, 0);
    ma_sound_set_looping(&keep_alive_sound, true);
    ma_sound_start(&keep_alive_sound);
#endif

    //
    // Midi input
    //

    RtMidiInPtr midiin = rtmidi_in_create_default();

    while (true)
    {
#if 0
        if (!ConnectToControl(506))
        {
            SleepMilliseconds(100);
            continue;
        }

        // control requested audio?

        u16 registers[1];
        ReadControlRegisters(0, 1, registers);
        WriteControlRegister(0, 123);
#endif

	if (TimerUpdate(&state.reset_timer, 0.016))
	{
	    state.sequence_index = 0;
	}

        // keyboard stuff...

        SelectMidiDevice(midiin);

        u8 message[1024];
        u64 message_size = sizeof(message);

        f64 stamp = rtmidi_in_get_message(midiin, message, (size_t *) &message_size);

        // https://www.songstuff.com/recording/article/midi-message-format/
        if (message_size)
        {
            u32 action = (message[0] & 0xF0) >> 4;

            // if (action == 0x8)
            // {
            //     u32 note_number = message[1];
            //     printf("Node Off: %u, %f\n", note_number, stamp);
            // }

            if (action == 0x9)
            {
                u32 note_number = message[1];
                printf("Pressed: %u\n", note_number);

                if (current_sound)
                {
                    ma_sound_stop(current_sound);
                    current_sound = NULL;
                }

                u32 current_note = 36;
                for (u32 i = 0; i < 29; ++i)
                {
                    if (current_note == note_number)
                    {
                        // we have to play sound i
                        current_sound = keyboard_sounds + i;
                        ma_sound_seek_to_pcm_frame(current_sound, 0);
                        ma_sound_start(current_sound);
			printf("Playing note: %u\n", current_note);
                        break;
                    }

                    current_note += octave_offsets[i % lengthof(octave_offsets)];
                }

                if (note_number == key_sequence[state.sequence_index])
                {
                    if (++state.sequence_index >= lengthof(key_sequence))
                    {
                        printf("Sequence completed\n");
                        state.sequence_index = 0;
                    }

		    TimerStart(&state.reset_timer, 10);
                }
                else
                {
		    state.reset_timer = {};
                    state.sequence_index = 0;
                }
            }
        }
        else
        {
            SleepMilliseconds(16);
        }
    }

    return 0;
}

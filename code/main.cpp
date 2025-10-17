#include <stdio.h>
#include <cstdlib>
#include <string.h>
#include "rtmidi_c.h"

#include "miniaudio.h"

#define UNIX
#include "common.h"

struct State
{
    u32 sequence_index;
};

State state;

const char *device_search = "Origin";

u32 key_sequence[] = {
    57,				// A
    59,				// B
    60,				// C
    62,				// D
    57,				// A
    64,				// E
    65,				// F
    67,				// G
};

const char *keyboard_sound_names[] = {
    "assets/L_Tetris.mp3",		// C
    NULL,				
    "assets/N_SevenNationArmy.mp3",	// D
    NULL,
    "assets/M_Tetris.mp3",		// E
    "assets/O_SevenNationArmy.mp3",	// F
    NULL,
    "assets/A_HWTH.wav",		// G
    NULL,
    "assets/D_HWTH.mp3",		// A
    NULL,
    "assets/C_HWTH.mp3",		// B
    "assets/E_HWTH.mp3",		// C
    NULL,
    "assets/B_HWTH.mp3",		// D
    NULL,
    "assets/F_HWTH.mp3",		// E
    "assets/G_HWTH.mp3",		// F
    NULL,
    "assets/H_AlleMeineEntchen.mp3",	// G
    NULL,
    "assets/I_AlleMeineEntchen.mp3",
    NULL,
    "assets/K_AlleMeineEntchen.mp3",
    "assets/J_AlleMeineEntchen.mp3",
};

int main()
{
    //
    // Audio
    //

    ma_engine audio;

    if (ma_engine_init(NULL, &audio) != MA_SUCCESS)
    {
	printf("Failed to initialize miniaudio\n");
	return -1;
    }

    ma_sound keyboard_sounds[lengthof(keyboard_sound_names)];
    memset(keyboard_sounds, 0, sizeof(keyboard_sounds));

    for (u32 i = 0; i < lengthof(keyboard_sounds); ++i)
    {
	if (keyboard_sound_names[i])
	{
	    // DECODE / No spacealization
	    u32 flags = 0x00004002;
	    ma_sound_init_from_file(&audio, keyboard_sound_names[i], flags, 
			    	    NULL, NULL, keyboard_sounds + i);
	}
    }

    ma_sound *current_sound = NULL;


    ma_sound_start(keyboard_sounds);

    //
    // Midi input
    //

    RtMidiInPtr midiin = rtmidi_in_create_default();

    printf("Searching for device...\n");

    char device_name[128];
    i32 device_index = -1;

    while (device_index < 0)
    {
        u32 port_count = rtmidi_get_port_count(midiin);

        for (u32 i = 0; i < port_count; ++i)
        {
            i32 device_name_length = sizeof(device_name);
            rtmidi_get_port_name(midiin, i, device_name, &device_name_length);

            bool match = true;
            for (u32 j = 0; device_search[j]; ++j)
            {
                if (j > device_name_length || device_search[j] != device_name[j])
                {
                    match = false;
                    break;
                }
            }

            if (match)
            {
                device_index = i;
                break;
            }
        }

        SleepMilliseconds(100);
    }

    printf("Selected device: %s\n", device_name);
    rtmidi_open_port(midiin, device_index, device_name);

    while (true)
    {
        u8 message[1024];
        u32 message_size = sizeof(message);

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

		u32 sound_id = note_number - 48;
		if (sound_id < lengthof(keyboard_sounds) && keyboard_sound_names[sound_id])
		{
		    current_sound = keyboard_sounds + sound_id;
		    ma_sound_seek_to_pcm_frame(current_sound, 0);
		    ma_sound_start(current_sound);
		}
                
                if (note_number == key_sequence[state.sequence_index])
                {
                    if (++state.sequence_index >= lengthof(key_sequence))
                    {
                        printf("Sequence completed\n");
                        state.sequence_index = 0;
                    }
                }
                else
                {
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

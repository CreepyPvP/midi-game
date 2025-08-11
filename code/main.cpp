#include <stdio.h>
#include <cstdlib>
#include "rtmidi_c.h"

#define WINDOWS
#include "common.h"

int main()
{
    RtMidiInPtr midiin = rtmidi_in_create_default();

    printf("Searching for device...\n");

    const char *device_search = "Origin";
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

            if (action == 0x8)
            {
                u32 note_number = message[1];
                printf("Node Off: %u, %f\n", note_number, stamp);
            }
            if (action == 0x9)
            {
                u32 note_number = message[1];
                printf("Node On: %u, %f\n", note_number, stamp);
            }
        }

        SleepMilliseconds(10);
    }

    return 0;
}

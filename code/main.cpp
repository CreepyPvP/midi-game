#include <stdio.h>
#include <cstdlib>
#include "rtmidi_c.h"

#define WINDOWS
#include "common.h"

int main()
{
    RtMidiInPtr midiin = rtmidi_in_create_default();

    printf("Searching for device...\n");

    while (!rtmidi_get_port_count(midiin))
    {
        SleepMilliseconds(100);
    }

    char device_name[128];
    i32 device_name_length = sizeof(device_name);
    rtmidi_get_port_name(midiin, 0, device_name, &device_name_length);

    printf("Selected device: %s\n", device_name);

    rtmidi_open_port(midiin, 0, device_name);

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

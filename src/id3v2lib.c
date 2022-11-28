/*
 * This file is part of id3v2lib library
 *
 * Copyright (c) Lars Ruiz
 *
 * For the full copyright and license information, please view the LICENSE
 * file that was distributed with this source code.
 */

#include <stdio.h>
#include <stdlib.h>

#include "frame.private.h"
#include "frame_list.private.h"
#include "id3v2lib.h"
#include "tag.private.h"
#include "tag_header.private.h"

ID3v2_tag* ID3v2_read_tag(const char* file_name)
{
    ID3v2_TagHeader* tag_header = ID3v2_TagHeader_read(file_name);

    if (tag_header == NULL) return NULL;

    FILE* file = fopen(file_name, "rb");
    int buffer_length = tag_header->tag_size + ID3v2_TAG_HEADER_LENGTH;
    char* tag_buffer = (char*) malloc((buffer_length) * sizeof(char));

    if (tag_buffer == NULL)
    {
        perror("Could not allocate buffer.");
        fclose(file);
        free(tag_header);
        return NULL;
    }

    fread(tag_buffer, buffer_length, 1, file);
    fclose(file);

    ID3v2_tag* tag = ID3v2_read_tag_from_buffer(tag_buffer, buffer_length);
    free(tag_buffer);

    return tag;
}

ID3v2_tag* ID3v2_read_tag_from_buffer(const char* tag_buffer, int buffer_length)
{
    ID3v2_TagHeader* tag_header = TagHeader_parse(tag_buffer);

    if (tag_header == NULL)
    {
        return NULL;
    }

    if (buffer_length < tag_header->tag_size + 10)
    {
        // The tag is bigger than the provided buffer size.
        free(tag_header);
        return NULL;
    }

    ID3v2_tag* tag = ID3v2_tag_new();
    tag->header = tag_header;

    int cursor = 0;
    cursor += ID3v2_TAG_HEADER_LENGTH; // Skip tag header

    if (tag->header->extended_header_size > 0)
    {
        // An extended header exists, skip it too
        cursor += tag->header->extended_header_size + ID3v2_EXTENDED_HEADED_SIZE_LENGTH;
    }

    ID3v2_frame* current_frame;

    while (cursor < tag->header->tag_size)
    {
        current_frame = frame_parse(tag_buffer + cursor, tag->header->major_version);

        if (current_frame == NULL)
        {
            break;
        }

        cursor += ID3v2_FRAME_HEADER_LENGTH + current_frame->header->size;
        FrameList_add_frame(tag->frames, current_frame);
    }

    tag->padding_size =
        (ID3v2_TAG_HEADER_LENGTH + tag->header->extended_header_size + tag->header->tag_size) -
        cursor;

    return tag;
}

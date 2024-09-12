// Lower and upper bounds for the number of bytes required to encode a value.
const u32 MINIMUM_ENCODING_BYTES = 100;
const u32 MAXIMUM_ENCODING_BYTES = 200;

u64 find_element(char* json, u64 length, u64 cursor, const char* name) {
    u64 strl = strlen(name);
    if (cursor+strl-1 >= length) {
        return length;
    }
    char* substr = strstr(json+cursor, name);
    if (!substr) {
        return length;
    }
    cursor = (u64)(substr - json) + strl;
    Assert(json[cursor] == ':');
    return cursor+1;
}

u64 find_char(char* json, u64 length, u64 cursor, char find) {
    if (cursor >= length) {
        return length;
    }
    do {
        if (json[cursor] == find) {
            return cursor+1;
        }
    } while (cursor++ < length);
    return cursor;
}

f64 parse_f64(char* json, u64 length, u64 cursor) {
    if (cursor >= length) {
        return 0.0;
    }
    return strtod(json+cursor, 0);
}

HaversineData get_points_array(FileReadData json_data, char* json, u64 cursor) {
    TIME_BANDWIDTH(__FUNCTION__, *json_data.file_size - cursor);

    Assert(json[cursor] == '[');
    cursor++;

    // Guess an upper bound on number of elements in the array.
    // Elements take at least 100 bytes to be encoded.
    u64 count_guess = *json_data.file_size / MINIMUM_ENCODING_BYTES;

    HaversineData data;
    data.x0 = (f64*)malloc(count_guess*sizeof(f64));
    data.y0 = (f64*)malloc(count_guess*sizeof(f64));
    data.x1 = (f64*)malloc(count_guess*sizeof(f64));
    data.y1 = (f64*)malloc(count_guess*sizeof(f64));
    data.count = count_guess;

    u64 length = json_data.chunk_size;
    u64 i = 0;
    b32 current_buffer0 = 1;
    b32 final_chunk = FALSE;
    while (TRUE) {
        // Check if next element is guaranteed to fit in remainder of chunk.
        // If not, and there are remaining chunks, transfer to the next chunk.
        // Copy the rest of the chunk into the extra space before the next chunk, mark it as complete, and read the next chunk into the completed chunk.
        // TODO: multithreading -- mark chunk as completed for read thread, spin until next chunk is ready if not ready.
        if (length - cursor < MAXIMUM_ENCODING_BYTES) {
            Assert(current_buffer0 ? !json_data.buffer1_complete : !json_data.buffer0_complete);
            u64 remainder_bytes = length - cursor;
            u8* next_buffer = current_buffer0 ? json_data.buffer1 : json_data.buffer0;
            next_buffer += json_data.extra_size - remainder_bytes;
            memcpy(next_buffer, json + cursor, remainder_bytes);
            if (*json_data.no_more_chunks) {
                final_chunk = TRUE;
            } else {
                // Queue read of next chunk.
                if (current_buffer0) {
                    *json_data.buffer0_complete = TRUE;
                } else {
                    *json_data.buffer1_complete = TRUE;
                }
            }
            current_buffer0 = !current_buffer0;

            json = (char*)next_buffer;
            cursor = 0;
            length = json_data.chunk_size + remainder_bytes;
            if (*json_data.no_more_chunks) final_chunk = TRUE; 
        }

        cursor = find_char(json, length, cursor, '{');
        if (cursor >= length) {
            if (final_chunk) {
                // We're done
                break;
            } else {
                // There are more chunks to come, skip to next loop to load next chunk.
                continue;
            }
        }
        cursor = find_element(json, length, cursor, "\"x0\"");
        data.x0[i] = parse_f64(json, length, cursor);
        cursor = find_element(json, length, cursor, "\"y0\"");
        data.y0[i] = parse_f64(json, length, cursor);
        cursor = find_element(json, length, cursor, "\"x1\"");
        data.x1[i] = parse_f64(json, length, cursor);
        cursor = find_element(json, length, cursor, "\"y1\"");
        data.y1[i] = parse_f64(json, length, cursor);
        cursor = find_char(json, length, cursor, '}');
        ++i;
    }
    data.count = i;
    return data;
}

HaversineData parse_haversine_json(FileReadData json_data) {
    Assert(json_data.extra_size > MAXIMUM_ENCODING_BYTES);

    // Assume "pairs" can be found in first chunk if it exists at all.
    u64 cursor = 0;
    char* json = (char*)json_data.buffer0 + json_data.extra_size;
    cursor = find_element(json, json_data.chunk_size, cursor, "\"pairs\"");
    if (cursor >= json_data.chunk_size) {
        Assert(!"Failed to find pairs!");
        return {};
    }

    HaversineData data = get_points_array(json_data, json, cursor);
    return data;
}
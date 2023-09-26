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

HaversineData get_points_array(char* json, u64 length, u64 cursor) {
    Assert(json[cursor] == '[');
    cursor++;

    // Find number of elements in array
    u64 count_cursor = cursor;
    u64 count = 0;
    while ((count_cursor < length) && (json[cursor] != ']')) {
        if (json[count_cursor++] == '{') {
            ++count;
        }
    }

    HaversineData data;
    data.x0 = (f64*)malloc(count*sizeof(f64));
    data.y0 = (f64*)malloc(count*sizeof(f64));
    data.x1 = (f64*)malloc(count*sizeof(f64));
    data.y1 = (f64*)malloc(count*sizeof(f64));
    data.count = count;

    for (u64 i = 0; i < count; ++i) {
        cursor = find_char(json, length, cursor, '{');
        cursor = find_element(json, length, cursor, "\"x0\"");
        data.x0[i] = parse_f64(json, length, cursor);
        cursor = find_element(json, length, cursor, "\"y0\"");
        data.y0[i] = parse_f64(json, length, cursor);
        cursor = find_element(json, length, cursor, "\"x1\"");
        data.x1[i] = parse_f64(json, length, cursor);
        cursor = find_element(json, length, cursor, "\"y1\"");
        data.y1[i] = parse_f64(json, length, cursor);
        cursor = find_char(json, length, cursor, '}');
        if (cursor >= length) {
            Assert(!"Error parsing pairs array.");
            return data;
        }
    }
    return data;
}

HaversineData parse_haversine_json(Bytes bytes) {
    u64 cursor = 0;
    char* json = (char*)bytes.buffer;
    u64 length = bytes.size;
    cursor = find_element(json, length, cursor, "\"pairs\"");
    if (cursor >= length) {
        Assert(!"Failed to find pairs!");
        return {};
    }

    HaversineData data = get_points_array(json, length, cursor);
    return data;
}
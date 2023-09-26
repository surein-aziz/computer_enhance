

// REFERENCE

static f64 square(f64 A)
{
    f64 Result = (A*A);
    return Result;
}

static f64 radians_from_degrees(f64 Degrees)
{
    f64 Result = 0.01745329251994329577f * Degrees;
    return Result;
}

// NOTE(casey): EarthRadius is generally expected to be 6372.8
static f64 reference_haversine(f64 X0, f64 Y0, f64 X1, f64 Y1, f64 earth_radius)
{
    /* NOTE(casey): This is not meant to be a "good" way to calculate the Haversine distance.
       Instead, it attempts to follow, as closely as possible, the formula used in the real-world
       question on which these homework exercises are loosely based.
    */
    
    f64 lat1 = Y0;
    f64 lat2 = Y1;
    f64 lon1 = X0;
    f64 lon2 = X1;
    
    f64 dLat = radians_from_degrees(lat2 - lat1);
    f64 dLon = radians_from_degrees(lon2 - lon1);
    lat1 = radians_from_degrees(lat1);
    lat2 = radians_from_degrees(lat2);
    
    f64 a = square(sin(dLat/2.0)) + cos(lat1)*cos(lat2)*square(sin(dLon/2));
    f64 c = 2.0*asin(sqrt(a));
    
    f64 Result = earth_radius * c;
    
    return Result;
}

// END REFERENCE

HaversineResult calculate_haversine(HaversineData data) {
    HaversineResult result;
    result.results = (f64*)malloc(data.count*sizeof(f64));
    result.count = data.count;

    // Use an estimate of the earth's radius for the radius, arbitrarily.
    f64 radius = 6372.8;
    f64 average = 0.0;
    for (u64 i = 0; i < result.count; ++i) {
        result.results[i] = reference_haversine(data.x0[i], data.y0[i], data.x1[i], data.y1[i], radius);
        average += result.results[i];
    }
    average = average / result.count;

    return result;
}
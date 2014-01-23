


#include <geocode-glib/geocode-glib.h>
#include <geocode-glib/geocode-enum-types.h>

/* enumerations from "geocode-location.h" */
GType
geocode_location_uri_scheme_get_type (void)
{
  static GType etype = 0;
  if (etype == 0) {
    static const GEnumValue values[] = {
      { GEOCODE_LOCATION_URI_SCHEME_GEO, "GEOCODE_LOCATION_URI_SCHEME_GEO", "geo" },
      { 0, NULL, NULL }
    };
    etype = g_enum_register_static ("GeocodeLocationURIScheme", values);
  }
  return etype;
}
GType
geocode_location_crs_get_type (void)
{
  static GType etype = 0;
  if (etype == 0) {
    static const GEnumValue values[] = {
      { GEOCODE_LOCATION_CRS_WGS84, "GEOCODE_LOCATION_CRS_WGS84", "wgs84" },
      { 0, NULL, NULL }
    };
    etype = g_enum_register_static ("GeocodeLocationCRS", values);
  }
  return etype;
}

/* enumerations from "geocode-error.h" */
GType
geocode_error_get_type (void)
{
  static GType etype = 0;
  if (etype == 0) {
    static const GEnumValue values[] = {
      { GEOCODE_ERROR_PARSE, "GEOCODE_ERROR_PARSE", "parse" },
      { GEOCODE_ERROR_NOT_SUPPORTED, "GEOCODE_ERROR_NOT_SUPPORTED", "not-supported" },
      { GEOCODE_ERROR_NO_MATCHES, "GEOCODE_ERROR_NO_MATCHES", "no-matches" },
      { GEOCODE_ERROR_INVALID_ARGUMENTS, "GEOCODE_ERROR_INVALID_ARGUMENTS", "invalid-arguments" },
      { GEOCODE_ERROR_INTERNAL_SERVER, "GEOCODE_ERROR_INTERNAL_SERVER", "internal-server" },
      { 0, NULL, NULL }
    };
    etype = g_enum_register_static ("GeocodeError", values);
  }
  return etype;
}

/* enumerations from "geocode-place.h" */
GType
geocode_place_type_get_type (void)
{
  static GType etype = 0;
  if (etype == 0) {
    static const GEnumValue values[] = {
      { GEOCODE_PLACE_TYPE_UNKNOWN, "GEOCODE_PLACE_TYPE_UNKNOWN", "unknown" },
      { GEOCODE_PLACE_TYPE_BUILDING, "GEOCODE_PLACE_TYPE_BUILDING", "building" },
      { GEOCODE_PLACE_TYPE_STREET, "GEOCODE_PLACE_TYPE_STREET", "street" },
      { GEOCODE_PLACE_TYPE_TOWN, "GEOCODE_PLACE_TYPE_TOWN", "town" },
      { GEOCODE_PLACE_TYPE_STATE, "GEOCODE_PLACE_TYPE_STATE", "state" },
      { GEOCODE_PLACE_TYPE_COUNTY, "GEOCODE_PLACE_TYPE_COUNTY", "county" },
      { GEOCODE_PLACE_TYPE_LOCAL_ADMINISTRATIVE_AREA, "GEOCODE_PLACE_TYPE_LOCAL_ADMINISTRATIVE_AREA", "local-administrative-area" },
      { GEOCODE_PLACE_TYPE_POSTAL_CODE, "GEOCODE_PLACE_TYPE_POSTAL_CODE", "postal-code" },
      { GEOCODE_PLACE_TYPE_COUNTRY, "GEOCODE_PLACE_TYPE_COUNTRY", "country" },
      { GEOCODE_PLACE_TYPE_ISLAND, "GEOCODE_PLACE_TYPE_ISLAND", "island" },
      { GEOCODE_PLACE_TYPE_AIRPORT, "GEOCODE_PLACE_TYPE_AIRPORT", "airport" },
      { GEOCODE_PLACE_TYPE_RAILWAY_STATION, "GEOCODE_PLACE_TYPE_RAILWAY_STATION", "railway-station" },
      { GEOCODE_PLACE_TYPE_BUS_STOP, "GEOCODE_PLACE_TYPE_BUS_STOP", "bus-stop" },
      { GEOCODE_PLACE_TYPE_MOTORWAY, "GEOCODE_PLACE_TYPE_MOTORWAY", "motorway" },
      { GEOCODE_PLACE_TYPE_DRAINAGE, "GEOCODE_PLACE_TYPE_DRAINAGE", "drainage" },
      { GEOCODE_PLACE_TYPE_LAND_FEATURE, "GEOCODE_PLACE_TYPE_LAND_FEATURE", "land-feature" },
      { GEOCODE_PLACE_TYPE_MISCELLANEOUS, "GEOCODE_PLACE_TYPE_MISCELLANEOUS", "miscellaneous" },
      { GEOCODE_PLACE_TYPE_SUPERNAME, "GEOCODE_PLACE_TYPE_SUPERNAME", "supername" },
      { GEOCODE_PLACE_TYPE_POINT_OF_INTEREST, "GEOCODE_PLACE_TYPE_POINT_OF_INTEREST", "point-of-interest" },
      { GEOCODE_PLACE_TYPE_SUBURB, "GEOCODE_PLACE_TYPE_SUBURB", "suburb" },
      { GEOCODE_PLACE_TYPE_COLLOQUIAL, "GEOCODE_PLACE_TYPE_COLLOQUIAL", "colloquial" },
      { GEOCODE_PLACE_TYPE_ZONE, "GEOCODE_PLACE_TYPE_ZONE", "zone" },
      { GEOCODE_PLACE_TYPE_HISTORICAL_STATE, "GEOCODE_PLACE_TYPE_HISTORICAL_STATE", "historical-state" },
      { GEOCODE_PLACE_TYPE_HISTORICAL_COUNTY, "GEOCODE_PLACE_TYPE_HISTORICAL_COUNTY", "historical-county" },
      { GEOCODE_PLACE_TYPE_CONTINENT, "GEOCODE_PLACE_TYPE_CONTINENT", "continent" },
      { GEOCODE_PLACE_TYPE_TIME_ZONE, "GEOCODE_PLACE_TYPE_TIME_ZONE", "time-zone" },
      { GEOCODE_PLACE_TYPE_ESTATE, "GEOCODE_PLACE_TYPE_ESTATE", "estate" },
      { GEOCODE_PLACE_TYPE_HISTORICAL_TOWN, "GEOCODE_PLACE_TYPE_HISTORICAL_TOWN", "historical-town" },
      { GEOCODE_PLACE_TYPE_OCEAN, "GEOCODE_PLACE_TYPE_OCEAN", "ocean" },
      { GEOCODE_PLACE_TYPE_SEA, "GEOCODE_PLACE_TYPE_SEA", "sea" },
      { 0, NULL, NULL }
    };
    etype = g_enum_register_static ("GeocodePlaceType", values);
  }
  return etype;
}




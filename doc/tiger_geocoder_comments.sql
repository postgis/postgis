
COMMENT ON FUNCTION Drop_Indexes_Generate_Script(text ) IS 'args: param_schema=tiger_data - Generates a script that drops all non-primary key and non-unique indexes on tiger schema and user specified schema. Defaults schema to tiger_data if no schema is specified.';
			
COMMENT ON FUNCTION Drop_State_Tables_Generate_Script(text , text ) IS 'args: param_state, param_schema=tiger_data - Generates a script that drops all tables in the specified schema that start with county_all, state_all or stae code followed by county or state.';
			
COMMENT ON FUNCTION Drop_State_Tables_Generate_Script(text , text ) IS 'args: param_state, param_schema=tiger_data - Generates a script that drops all tables in the specified schema that are prefixed with the state abbreviation. Defaults schema to tiger_data if no schema is specified.';
			
COMMENT ON FUNCTION geocode(varchar , integer , geometry ) IS 'args: address, max_results=10, restrict_region=NULL, OUT addy, OUT geomout, OUT rating - Takes in an address as a string (or other normalized address) and outputs a set of possible locations which include a point geometry in NAD 83 long lat, a normalized address for each, and the rating. The lower the rating the more likely the match. Results are sorted by lowest rating first. Can optionally pass in maximum results, defaults to 10, and restrict_region (defaults to NULL)';
			
COMMENT ON FUNCTION geocode(norm_addy , integer , geometry ) IS 'args: in_addy, max_results=10, restrict_region=NULL, OUT addy, OUT geomout, OUT rating - Takes in an address as a string (or other normalized address) and outputs a set of possible locations which include a point geometry in NAD 83 long lat, a normalized address for each, and the rating. The lower the rating the more likely the match. Results are sorted by lowest rating first. Can optionally pass in maximum results, defaults to 10, and restrict_region (defaults to NULL)';
			
COMMENT ON FUNCTION geocode_intersection(text , text , text , text , text , integer ) IS 'args:  roadway1,  roadway2,  in_state,  in_city,  in_zip, max_results=10, OUT addy, OUT geomout, OUT rating - Takes in 2 streets that intersect and a state, city, zip, and outputs a set of possible locations on the first cross street that is at the intersection, also includes a point geometry in NAD 83 long lat, a normalized address for each location, and the rating. The lower the rating the more likely the match. Results are sorted by lowest rating first. Can optionally pass in maximum results, defaults to 10';
			
COMMENT ON FUNCTION Get_Geocode_Setting(text ) IS 'args:  setting_name - Returns value of specific setting stored in tiger.geocode_settings table.';
			
COMMENT ON FUNCTION get_tract(geometry , text ) IS 'args:  loc_geom,  output_field=name - Returns census tract or field from tract table of where the geometry is located. Default to returning short name of tract.';
			
COMMENT ON FUNCTION Install_Missing_Indexes() IS 'Finds all tables with key columns used in geocoder joins and filter conditions that are missing used indexes on those columns and will add them.';
			
COMMENT ON FUNCTION loader_generate_census_script(text[], text) IS 'args: param_states, os - Generates a shell script for the specified platform for the specified states that will download Tiger census state tract, bg, and tabblocks data tables, stage and load into tiger_data schema. Each state script is returned as a separate record.';
			
COMMENT ON FUNCTION loader_generate_script(text[], text) IS 'args: param_states, os - Generates a shell script for the specified platform for the specified states that will download Tiger data, stage and load into tiger_data schema. Each state script is returned as a separate record. Latest version supports Tiger 2010 structural changes and also loads census tract, block groups, and blocks tables.';
			
COMMENT ON FUNCTION loader_generate_nation_script(text) IS 'args: os - Generates a shell script for the specified platform that loads in the county and state lookup tables.';
			
COMMENT ON FUNCTION Missing_Indexes_Generate_Script() IS 'Finds all tables with key columns used in geocoder joins that are missing indexes on those columns and will output the SQL DDL to define the index for those tables.';
			
COMMENT ON FUNCTION normalize_address(varchar ) IS 'args: in_address - Given a textual street address, returns a composite norm_addy type that has road suffix, prefix and type standardized, street, streetname etc. broken into separate fields. This function will work with just the lookup data packaged with the tiger_geocoder (no need for tiger census data).';
			
COMMENT ON FUNCTION pagc_normalize_address(varchar ) IS 'args: in_address - Given a textual street address, returns a composite norm_addy type that has road suffix, prefix and type standardized, street, streetname etc. broken into separate fields. This function will work with just the lookup data packaged with the tiger_geocoder (no need for tiger census data). Requires address_standardizer extension.';
			
COMMENT ON FUNCTION pprint_addy(norm_addy ) IS 'args: in_addy - Given a norm_addy composite type object, returns a pretty print representation of it. Usually used in conjunction with normalize_address.';
			
COMMENT ON FUNCTION Reverse_Geocode(geometry , boolean ) IS 'args: pt, include_strnum_range=false, OUT intpt, OUT addy, OUT street - Takes a geometry point in a known spatial ref sys and returns a record containing an array of theoretically possible addresses and an array of cross streets. If include_strnum_range = true, includes the street range in the cross streets.';
			
COMMENT ON FUNCTION Topology_Load_Tiger(varchar , varchar , varchar ) IS 'args: topo_name, region_type, region_id - Loads a defined region of tiger data into a PostGIS Topology and transforming the tiger data to spatial reference of the topology and snapping to the precision tolerance of the topology.';
			
COMMENT ON FUNCTION Set_Geocode_Setting(text , text ) IS 'args:  setting_name,  setting_value - Sets a setting that affects behavior of geocoder functions.';
			
CREATE OR REPLACE FUNCTION tiger.standardize_tiger_data(param_states varchar[]) RETURNS void AS
$$
-- fix park addresses, normalizer puts this in suftype
UPDATE tiger.featnames SET name = replace(name,' Park', ''), suftypabrv = 'Park' where statefp IN (SELECT statefp FROM tiger.state_lookup WHERE abbrev = ANY(param_states) )
     AND name ILIKE '% Park' AND suftypabrv is null;

-- spell out Saint
-- fix park addresses, normalizer puts this in suftype
UPDATE tiger.featnames SET name = replace(name,'St ', 'Saint ') where statefp IN (SELECT statefp FROM tiger.state_lookup WHERE abbrev = ANY(param_states) )
     AND name ILIKE 'St %';

$$
language 'sql';

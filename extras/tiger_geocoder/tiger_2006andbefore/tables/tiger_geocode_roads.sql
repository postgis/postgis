DROP SEQUENCE IF EXISTS tiger_geocode_roads_seq;
CREATE SEQUENCE tiger_geocode_roads_seq;

DROP TABLE IF EXISTS tiger_geocode_roads;
CREATE TABLE tiger_geocode_roads (
    id      INTEGER,
    tlid    INTEGER,
    fedirp  VARCHAR(2),
    fename  VARCHAR(30),
    fetype  VARCHAR(4),
    fedirs  VARCHAR(2),
    zip     INTEGER,
    state   VARCHAR(2),
    county  VARCHAR(90),
    cousub  VARCHAR(90),
    place   VARCHAR(90)
);

INSERT INTO tiger_geocode_roads
  SELECT
    nextval('tiger_geocode_roads_seq'),
    tlid,
    fedirp,
    fename,
    fetype,
    fedirs,
    zip,
    state,
    county,
    cousub,
    place
  FROM
   (SELECT
      tlid,
      fedirp,
      fename,
      fetype,
      fedirs,
      zipl as zip,
      sl.abbrev as state,
      co.name as county,
      cs.name as cousub,
      pl.name as place
    FROM
      roads_local rl
      JOIN state_lookup sl on (rl.statel = sl.st_code)
      LEFT JOIN county_lookup co on (rl.statel = co.st_code AND rl.countyl = co.co_code)
      LEFT JOIN countysub_lookup cs on (rl.statel = cs.st_code AND rl.countyl = cs.co_code AND rl.cousubl = cs.cs_code)
      LEFT JOIN place_lookup pl on (rl.statel = pl.st_code AND rl.placel = pl.pl_code)
    WHERE fename IS NOT NULL
    UNION
    SELECT
      tlid,
      fedirp,
      fename,
      fetype,
      fedirs,
      zipr as zip,
      sl.abbrev as state,
      co.name as county,
      cs.name as cousub,
      pl.name as place
    FROM
      roads_local rl
      JOIN state_lookup sl on (rl.stater = sl.st_code)
      LEFT JOIN county_lookup co on (rl.stater = co.st_code AND rl.countyr = co.co_code)
      LEFT JOIN countysub_lookup cs on (rl.stater = cs.st_code AND rl.countyr = cs.co_code AND rl.cousubr = cs.cs_code)
      LEFT JOIN place_lookup pl on (rl.stater = pl.st_code AND rl.placer = pl.pl_code)
    WHERE fename IS NOT NULL
    ) AS sub;

CREATE INDEX tiger_geocode_roads_zip_soundex_idx          ON tiger_geocode_roads (soundex(fename), zip, state);
CREATE INDEX tiger_geocode_roads_place_soundex_idx        ON tiger_geocode_roads (soundex(fename), place, state);
CREATE INDEX tiger_geocode_roads_cousub_soundex_idx       ON tiger_geocode_roads (soundex(fename), cousub, state);
CREATE INDEX tiger_geocode_roads_place_more_soundex_idx   ON tiger_geocode_roads (soundex(fename), soundex(place), state);
CREATE INDEX tiger_geocode_roads_cousub_more_soundex_idx  ON tiger_geocode_roads (soundex(fename), soundex(cousub), state);
CREATE INDEX tiger_geocode_roads_state_soundex_idx        ON tiger_geocode_roads (soundex(fename), state);


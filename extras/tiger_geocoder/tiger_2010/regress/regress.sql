\a
--SET seq_page_cost='1000';
\o normalize_address_regress.out
\i normalize_address_regress.sql
\o geocode_regress.out
\i geocode_regress.sql
\o reverse_geocode_regress.out
\i reverse_geocode_regress.sql
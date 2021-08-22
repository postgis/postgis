set search_path=public,postgis,tiger,contrib,topology;
SELECT tiger.install_missing_indexes();

-- #6063: Application tables can share the names of topology catalogs.
CREATE SCHEMA restore_app;
CREATE TABLE restore_app.layer (id integer PRIMARY KEY);
INSERT INTO restore_app.layer VALUES (1), (2);

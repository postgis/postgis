-- #6063: Application tables can share the names of topology catalogs.
CREATE SCHEMA restore_app;
CREATE TABLE restore_app.layer (id integer PRIMARY KEY);
INSERT INTO restore_app.layer VALUES (1), (2);
CREATE TABLE public.layer (id integer PRIMARY KEY);
INSERT INTO public.layer VALUES (3), (4);
CREATE TABLE public.topology (id serial PRIMARY KEY, name text UNIQUE);
INSERT INTO public.topology (name) VALUES ('first'), ('second');
GRANT SELECT ON public.topology TO PUBLIC;

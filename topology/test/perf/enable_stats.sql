-- See https://www.postgresql.org/docs/current/pgstatstatements.html
CREATE EXTENSION IF NOT EXISTS pg_stat_statements;
SET pg_stat_statements.track to 'all';
SELECT pg_stat_statements_reset();

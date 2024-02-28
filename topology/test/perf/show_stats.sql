-- See https://www.postgresql.org/docs/current/pgstatstatements.html
SELECT
  calls,
  round(total_exec_time) tot_ms,
  substring(
    regexp_replace(
      regexp_replace(query,'\n',' ','g'),
      '  *',
      ' ',
      'g'
    )
    from 1 for 90
  ) query
FROM pg_stat_statements
ORDER BY total_exec_time desc;

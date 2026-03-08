SELECT je->>'pos' AS pos, je->>'input-word' AS input_word, je->>'input-token' AS input_token, je->>'mapped-word' AS ouput_word, je->>'output-token' AS output_token FROM jsonb(debug_standardize_address('us_lex'::text, 'us_gaz'::text, 'us_rules'::text, '123 Main Street'::text, 'Kansas City, MO 45678'::text)) AS d, jsonb_array_elements(d->'rules'->0->'rule_tokens' ) AS je;
SELECT jsonb_array_length(jsonb(d)->'rules') FROM debug_standardize_address('us_lex','us_gaz','us_rules', '10-20 DORRANCE ST PROVIDENCE RI') AS d;

SELECT '#5299a' AS ticket, jsonb_array_length(d->'rules') AS num_rules, jsonb_array_length(d->'input_tokens') AS num_input_tokens,  d->'rules'->0->'raw_score' AS best_score FROM jsonb(debug_standardize_address('us_lex',  'us_gaz', 'us_rules','1 Timepiece Point','Boston, MA, 02220')) As d;

SELECT '#2978c' As ticket, jsonb_array_length(d->'rules') AS num_rules, jsonb_array_length(d->'input_tokens') AS num_input_tokens,  d->'rules'->0->'raw_score' AS best_score FROM jsonb(debug_standardize_address('us_lex','us_gaz','us_rules', '10-20 DORRANCE ST, PROVIDENCE, RI')) AS d;

SELECT '#5299b' AS ticket, jsonb_array_length(d->'rules') AS num_rules, jsonb_array_length(d->'input_tokens') AS num_input_tokens, d->'rules'->0->'score' AS best_score FROM jsonb(debug_standardize_address('us_lex',  'us_gaz', 'us_rules','50 Gold Piece Drive, Boston, MA, 02020')) AS d;

SELECT '#5299bt' AS ticket, it->>'pos' AS pos, it->>'word' AS word,  it->>'token' AS token, it->>'stdword' AS stdword FROM jsonb(debug_standardize_address('us_lex',  'us_gaz', 'us_rules','50 Gold Piece Drive, Boston, MA, 02020')) AS d, jsonb_array_elements(d->'input_tokens') AS it
WHERE it @> '{"pos": 2}'::jsonb
ORDER BY pos, stdword, word, token;

SELECT r->>'score' AS score, r->>'rule_type' AS rule_type, r->>'rule_weight' AS rule_weight, r->>'rule_string' AS rule, r->>'rule_stub_string' AS rule_stub FROM jsonb(debug_standardize_address('us_lex','us_gaz','us_rules', '25 Prince Street, NC 09985')) AS d, jsonb_array_elements(d->'rules') AS r;

SELECT '#state_only_macro_5arg' AS ticket,
       d->'stdaddr'->>'city' AS city,
       d->'stdaddr'->>'state' AS state,
       d->'stdaddr'->>'postcode' AS postcode
FROM jsonb(debug_standardize_address('us_lex','us_gaz','us_rules', '25 Prince Street', 'NC 09985')) AS d;

WITH d AS (
	SELECT jsonb(debug_standardize_address('us_lex','us_gaz','us_rules', '123 MAIN STREET APARTMENT 1 BUILDING A FLOOR 2 UNIT 3 ROOM 4', 'BOSTON MA 02101')) AS d
),
r AS (
	SELECT value AS rule
	FROM d, jsonb_array_elements(d->'rules')
)
SELECT
	(SELECT jsonb_array_length(d->'input_tokens') FROM d) AS num_input_tokens,
	(SELECT jsonb_array_length(d->'rules') FROM d) AS num_rules,
	count(*) FILTER (WHERE rule->'rule_stub_string' = 'null'::jsonb) AS skipped_rule_lookups,
	count(*) FILTER (WHERE rule->'rule_id' = 'null'::jsonb) AS null_rule_ids,
	count(*) FILTER (WHERE rule->>'rule_id' = '-1') AS unmatched_rule_rows
FROM r;

WITH d AS (
	SELECT jsonb(debug_standardize_address('us_lex','us_gaz','us_rules', '123 MAIN STREET APARTMENT 1 BUILDING A FLOOR 2 UNIT 3 ROOM 4', 'BOSTON MA 02101')) AS d
),
rule_tokens AS (
	SELECT je->>'output-token' AS output_token
	FROM d, jsonb_array_elements(d->'rules') AS r, jsonb_array_elements(r->'rule_tokens') AS je
)
SELECT
	count(*) FILTER (WHERE output_token = 'NONE') AS none_output_tokens,
	count(*) FILTER (WHERE output_token = 'INVALID') AS invalid_output_tokens
FROM rule_tokens;

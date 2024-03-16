SELECT je->>'pos' AS pos, je->>'input-word' AS input_word, je->>'input-token' AS input_token, je->>'mapped-word' AS ouput_word, je->>'output-token' AS output_token FROM jsonb(debug_standardize_address('us_lex'::text, 'us_gaz'::text, 'us_rules'::text, '123 Main Street'::text, 'Kansas City, MO 45678'::text)) AS d, jsonb_array_elements(d->'rules'->0->'rule_tokens' ) AS je;
SELECT jsonb_array_length(jsonb(d)->'rules') FROM debug_standardize_address('us_lex','us_gaz','us_rules', '10-20 DORRANCE ST PROVIDENCE RI') AS d;

SELECT '#5299a' AS ticket, jsonb_array_length(d->'rules') AS num_rules, jsonb_array_length(d->'input_tokens') AS num_input_tokens,  d->'rules'->0->'raw_score' AS best_score FROM jsonb(debug_standardize_address('us_lex',  'us_gaz', 'us_rules','1 Timepiece Point','Boston, MA, 02220')) As d;

SELECT '#2978c' As ticket, jsonb_array_length(d->'rules') AS num_rules, jsonb_array_length(d->'input_tokens') AS num_input_tokens,  d->'rules'->0->'raw_score' AS best_score FROM jsonb(debug_standardize_address('us_lex','us_gaz','us_rules', '10-20 DORRANCE ST, PROVIDENCE, RI')) AS d;

SELECT '#5299b' AS ticket, jsonb_array_length(d->'rules') AS num_rules, jsonb_array_length(d->'input_tokens') AS num_input_tokens, d->'rules'->0->'score' AS best_score FROM jsonb(debug_standardize_address('us_lex',  'us_gaz', 'us_rules','50 Gold Piece Drive, Boston, MA, 02020')) AS d;

SELECT '#5299bt' AS ticket, it->>'pos' AS pos, it->>'word' AS word,  it->>'token' AS token, it->>'stdword' AS stdword FROM jsonb(debug_standardize_address('us_lex',  'us_gaz', 'us_rules','50 Gold Piece Drive, Boston, MA, 02020')) AS d, jsonb_array_elements(d->'input_tokens') AS it
WHERE it @> '{"pos": 2}'::jsonb
ORDER BY pos, stdword, word, token;

SELECT r->>'score' AS score,r->>'rule_string' AS rule, r->>'rule_stub_string' AS rule_stub  FROM jsonb(debug_standardize_address('us_lex','us_gaz','us_rules', '25 Prince Street, NC 09985')) AS d, jsonb_array_elements(d->'rules') AS r;

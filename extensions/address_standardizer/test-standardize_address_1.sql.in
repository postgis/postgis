select * from parse_address('123 Main Street, Kansas City, MO 45678');
select * from standardize_address('us_lex'::text, 'us_gaz'::text, 'us_rules'::text, '123 Main Street'::text, 'Kansas City, MO 45678'::text);
SELECT '#2981' As ticket, * FROM standardize_address('us_lex','us_gaz','us_rules', '1566 NEW STATE HWY, RAYNHAM, MA');
SELECT '#2978a' As ticket, * FROM standardize_address('us_lex','us_gaz','us_rules', '10-20 DORRANCE ST PROVIDENCE RI' );
SELECT '#2978b' As ticket, * FROM standardize_address('us_lex','us_gaz','us_rules', '10 20 DORRANCE ST PROVIDENCE RI' );
SELECT '#2978c' As ticket, * FROM standardize_address('us_lex','us_gaz','us_rules', '10-20 DORRANCE ST, PROVIDENCE, RI');
SELECT '#5299a' AS ticket, * FROM standardize_address('us_lex',  'us_gaz', 'us_rules','1 Timepiece Point','Boston, MA, 02220');
SELECT '#5299b' AS ticket, * FROM standardize_address('us_lex',  'us_gaz', 'us_rules','50 Gold Piece Drive','Boston, MA, 02020');

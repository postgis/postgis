\timing
SELECT * FROM normalize_address('3937 43RD AVE S, MINNEAPOLIS, MN 55406');
-- comma in wrong spot
SELECT * FROM normalize_address('529 Main Street, Boston MA, 02129');
-- comma in right spot
SELECT * FROM normalize_address('529 Main Street, Boston MA, 02129');
-- partial address
SELECT * FROM normalize_address('529 Main Street, Boston MA');
\timing
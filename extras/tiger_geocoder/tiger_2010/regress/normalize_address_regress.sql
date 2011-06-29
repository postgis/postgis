\timing
SELECT * FROM normalize_address('3937 43RD AVE S, MINNEAPOLIS, MN 55406');
-- comma in wrong spot
SELECT * FROM normalize_address('529 Main Street, Boston MA, 02129');
-- comma in right spot
SELECT * FROM normalize_address('529 Main Street, Boston,MA 02129');
-- partial address
SELECT * FROM normalize_address('529 Main Street, Boston, MA');
-- partial address
SELECT * FROM normalize_address('529 Main Street, Boston, MA 021');

-- Mangled zipcodes
SELECT * FROM normalize_address('212 3rd Ave N, MINNEAPOLIS, MN 553404');
SELECT * FROM normalize_address('212 3rd Ave N, MINNEAPOLIS, MN 55401-');
\timing
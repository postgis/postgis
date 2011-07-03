--$Id$
\timing
SELECT '#887' As ticket, * FROM normalize_address('2450 N COLORADO ST, PHILADELPHIA, PA, 19132');
SELECT '#1051' As ticket, * FROM normalize_address('212 3rd Ave N Suite 560, Minneapolis, MN 55401');
SELECT '#1051' As ticket, * FROM normalize_address('3937 43RD AVE S, MINNEAPOLIS, MN 55406');
SELECT '#1051' As ticket, * FROM normalize_address('212 N 3rd Ave, Minneapolis, MN 55401');
-- City missing ,  -- NOTE this one won't normalize right if you don't have MN data loaded
SELECT '#1051' As ticket, * FROM normalize_address('212 3rd Ave N Minneapolis, MN 55401'); 
-- comma in wrong spot
SELECT * FROM normalize_address('529 Main Street, Boston MA, 02129');
-- comma in right spot
SELECT * FROM normalize_address('529 Main Street, Boston,MA 02129');
-- partial address
SELECT * FROM normalize_address('529 Main Street, Boston, MA');

-- Partial and Mangled zipcodes
SELECT '#1073' As ticket, * FROM normalize_address('212 3rd Ave N, MINNEAPOLIS, MN 553404');
SELECT '#1073' As ticket, * FROM normalize_address('212 3rd Ave N, MINNEAPOLIS, MN 55401-');
SELECT '#1073' As ticket, * FROM normalize_address('529 Main Street, Boston, MA 021');

-- comma in wrong position
SELECT '#1086' As ticket, * FROM normalize_address('949 N 3rd St, New Hyde Park, NY, 11040');

-- comma in right position --
SELECT '#1086' As ticket, * FROM normalize_address('949 N 3rd St, New Hyde Park, NY 11040');
\timing

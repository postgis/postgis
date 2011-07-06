--$Id$
\timing
SELECT '#887' As ticket, * FROM normalize_address('2450 N COLORADO ST, PHILADELPHIA, PA, 19132');
SELECT '#1051a' As ticket, * FROM normalize_address('212 3rd Ave N Suite 560, Minneapolis, MN 55401');
SELECT '#1051b' As ticket, * FROM normalize_address('3937 43RD AVE S, MINNEAPOLIS, MN 55406');
SELECT '#1051c' As ticket, * FROM normalize_address('212 N 3rd Ave, Minneapolis, MN 55401');
-- City missing ,  -- NOTE this one won't normalize right if you don't have MN data loaded
SELECT '#1051d' As ticket, * FROM normalize_address('212 3rd Ave N Minneapolis, MN 55401'); 
-- comma in wrong spot
SELECT * FROM normalize_address('529 Main Street, Boston MA, 02129');
-- comma in right spot
SELECT * FROM normalize_address('529 Main Street, Boston,MA 02129');
-- partial address
SELECT * FROM normalize_address('529 Main Street, Boston, MA');
-- Full address with suite using ,
SELECT * FROM normalize_address('529 Main Street, Apt 201, Boston, MA 02129');
-- Full address with apart using space
SELECT * FROM normalize_address('529 Main Street Apt 201, Boston, MA 02129');
-- Partial address with apartment
SELECT * FROM normalize_address('529 Main Street, Apt 201, Boston, MA');

-- Partial and Mangled zipcodes
SELECT '#1073a' As ticket, * FROM normalize_address('212 3rd Ave N, MINNEAPOLIS, MN 553404');
SELECT '#1073b' As ticket, * FROM normalize_address('212 3rd Ave N, MINNEAPOLIS, MN 55401-');
SELECT '#1073c' As ticket, * FROM normalize_address('529 Main Street, Boston, MA 021');

-- comma in wrong position
SELECT '#1086a' As ticket, * FROM normalize_address('949 N 3rd St, New Hyde Park, NY, 11040');

-- comma in right position --
SELECT '#1086b' As ticket, * FROM normalize_address('949 N 3rd St, New Hyde Park, NY 11040');

-- country roads and highways with spaces in street type
SELECT '#1076a' As ticket, * FROM normalize_address('16725 Co Rd 24, Plymouth, MN 55447'); 
SELECT '#1076b' As ticket, * FROM normalize_address('16725 County Road 24, Plymouth, MN 55447'); 
SELECT '#1076c' As ticket, * FROM normalize_address('13800 County Hwy 9, Andover, MN 55304');
SELECT '#1076d' As ticket, * FROM normalize_address('13800 9, Andover, MN 55304');
-- this one is a regular street that happens to have a street type as the name
SELECT '#1076e' As ticket, * FROM normalize_address('14 Forest Road, Acton, MA');

-- A country road with a letter name and direction 
-- NOTE this doesn't completely normalize right since the direction W is being cut off --
SELECT '#1076f' As ticket, * FROM normalize_address('1940 County Road C W, Roseville, MN 55113'); 

-- Route with a name that sounds like a direction --
SELECT '#1076g' As ticket, * FROM normalize_address('3900 Route 6, Eastham, Massachusetts 02642');

-- Street that has same name as type  --
SELECT '#1076h' As ticket, * FROM normalize_address('4533 PARK AVE S, MINNEAPOLIS, MN 55407');

-- same street with alternate county name
SELECT '#1076i' As ticket, * FROM normalize_address('4533 County Road 33, MINNEAPOLIS, MN 55407'); 

-- Same case of street type that has name as a type --
-- this matches -
SELECT '#1109a' As ticket, * from normalize_address('4373 LAKE DRIVE, ROBBINSDALE, MN 55422');

-- this failed --
SELECT '#1109b' As ticket, * from normalize_address('4373 LAKE DR, ROBBINSDALE, MN 55422');

-- this failed
SELECT '#1109b' As ticket, * from normalize_address('4373 LAKE DR, ROBBINSDALE, MN 55422');

\timing

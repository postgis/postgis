\pset pager off

drop table if exists test_parse_address cascade;
create table test_parse_address (
    id serial not null primary key,
    instring text not null,
    outstring text
);
copy test_parse_address (instring, outstring) from stdin;
	(,,,,,,,,)
 , ,  	(,,,,,,,,)
@@ sttype dirs dirs words$	
123 oak ln e n mycity ny	(123,"oak ln e",,"123 oak ln e","n mycity",NY,,,US)
123 oak lane east n mycity ny	(123,"oak lane east",,"123 oak lane east","n mycity",NY,,,US)
123 oak ln e north mycity ny	(123,"oak ln e",,"123 oak ln e","north mycity",NY,,,US)
@@ sttype dirs dirs saint words$	
123 oak ln e n st marie ny	(123,"oak ln e",,"123 oak ln e","n st marie",NY,,,US)
123 oak lane east n st marie ny	(123,"oak lane east",,"123 oak lane east","n st marie",NY,,,US)
123 oak ln e north st marie ny	(123,"oak ln e",,"123 oak ln e","north st marie",NY,,,US)
123 oak ln e n saint marie ny	(123,"oak ln e",,"123 oak ln e","n saint marie",NY,,,US)
123 oak lane east n saint marie ny	(123,"oak lane east",,"123 oak lane east","n saint marie",NY,,,US)
123 oak ln e north saint marie ny	(123,"oak ln e",,"123 oak ln e","north saint marie",NY,,,US)
@@ sttype dirs saint words$	
123 oak ln e st marie ny	(123,"oak ln",,"123 oak ln","e st marie",NY,,,US)
123 oak lane east st marie ny	(123,"oak lane",,"123 oak lane","east st marie",NY,,,US)
123 oak ln e st marie ny	(123,"oak ln",,"123 oak ln","e st marie",NY,,,US)
123 oak ln e saint marie ny	(123,"oak ln",,"123 oak ln","e saint marie",NY,,,US)
123 oak lane east saint marie ny	(123,"oak lane",,"123 oak lane","east saint marie",NY,,,US)
123 oak ln e saint marie ny	(123,"oak ln",,"123 oak ln","e saint marie",NY,,,US)
@@ sttype saint words$	
123 oak ln st marie ny	(123,"oak ln",,"123 oak ln","st marie",NY,,,US)
123 oak lane st marie ny	(123,"oak lane",,"123 oak lane","st marie",NY,,,US)
123 oak ln st marie ny	(123,"oak ln",,"123 oak ln","st marie",NY,,,US)
123 oak ln saint marie ny	(123,"oak ln",,"123 oak ln","saint marie",NY,,,US)
123 oak lane saint marie ny	(123,"oak lane",,"123 oak lane","saint marie",NY,,,US)
123 oak ln saint marie ny	(123,"oak ln",,"123 oak ln","saint marie",NY,,,US)
@@ sttype words$	
123 oak ln marie ny	(123,"oak ln",,"123 oak ln",marie,NY,,,US)
123 oak ln new marie ny	(123,"oak ln",,"123 oak ln","new marie",NY,,,US)
@@ === same as above but with commas ===	
@@ sttype dirs dirs words$	
123 oak ln e, n mycity ny	(123,"oak ln e",,"123 oak ln e","n mycity",NY,,,US)
123 oak lane east, n mycity ny	(123,"oak lane east",,"123 oak lane east","n mycity",NY,,,US)
123 oak ln e, north mycity ny	(123,"oak ln e",,"123 oak ln e","north mycity",NY,,,US)
123 oak ln e n, mycity ny	(123,"oak ln e n",,"123 oak ln e n",mycity,NY,,,US)
123 oak lane east n, mycity ny	(123,"oak lane east n",,"123 oak lane east n",mycity,NY,,,US)
123 oak ln e north, mycity ny	(123,"oak ln e north",,"123 oak ln e north",mycity,NY,,,US)
@@ sttype dirs dirs saint words$	
123 oak ln e, n st marie ny	(123,"oak ln e",,"123 oak ln e","n st marie",NY,,,US)
123 oak lane east, n st marie ny	(123,"oak lane east",,"123 oak lane east","n st marie",NY,,,US)
123 oak ln e, north st marie ny	(123,"oak ln e",,"123 oak ln e","north st marie",NY,,,US)
123 oak ln e, n saint marie ny	(123,"oak ln e",,"123 oak ln e","n saint marie",NY,,,US)
123 oak lane east, n saint marie ny	(123,"oak lane east",,"123 oak lane east","n saint marie",NY,,,US)
123 oak ln e, north saint marie ny	(123,"oak ln e",,"123 oak ln e","north saint marie",NY,,,US)
@@ sttype dirs saint words$	
123 oak ln e, st marie ny	(123,"oak ln e",,"123 oak ln e","st marie",NY,,,US)
123 oak lane east, st marie ny	(123,"oak lane east",,"123 oak lane east","st marie",NY,,,US)
123 oak ln e, st marie ny	(123,"oak ln e",,"123 oak ln e","st marie",NY,,,US)
123 oak ln e, saint marie ny	(123,"oak ln e",,"123 oak ln e","saint marie",NY,,,US)
123 oak lane east, saint marie ny	(123,"oak lane east",,"123 oak lane east","saint marie",NY,,,US)
123 oak ln e, saint marie ny	(123,"oak ln e",,"123 oak ln e","saint marie",NY,,,US)
@@ sttype saint words$	
123 oak ln, st marie ny	(123,"oak ln",,"123 oak ln","st marie",NY,,,US)
123 oak lane, st marie ny	(123,"oak lane",,"123 oak lane","st marie",NY,,,US)
123 oak ln, st marie ny	(123,"oak ln",,"123 oak ln","st marie",NY,,,US)
123 oak ln, saint marie ny	(123,"oak ln",,"123 oak ln","saint marie",NY,,,US)
123 oak lane, saint marie ny	(123,"oak lane",,"123 oak lane","saint marie",NY,,,US)
123 oak ln, saint marie ny	(123,"oak ln",,"123 oak ln","saint marie",NY,,,US)
@@ sttype words$	
123 oak ln, marie ny	(123,"oak ln",,"123 oak ln",marie,NY,,,US)
123 oak ln, new marie ny	(123,"oak ln",,"123 oak ln","new marie",NY,,,US)
529 Main Street, Boston, MA 021	(529,"Main Street",,"529 Main Street",Boston,MA,021,,US)
529 Main Street, Boston, MA 021, France	(529,"Main Street",,"529 Main Street",Boston,MA,021,,FR)
100 Peachtree St, Atlanta, Georgia	(100,"Peachtree St",,"100 Peachtree St",Atlanta,GA,,,US)
212 n 3rd ave, Minneapolis, mn 55401, USA	(212,"n 3rd ave",,"212 n 3rd ave",Minneapolis,MN,55401,,US)
26 Court Street, Boston, Massachusetts 02109, FR	(26,"Court Street",,"26 Court Street",Boston,MA,02109,,FR)
26 Court Street, Boston, MA 02109, Rear	(26,"Court Street, Boston, MA 02109",,"26 Court Street, Boston, MA 02109",Rear,,,,US)
212 N 3rd Ave, Minneapolis, MN 55401, Suite A	(212,"N 3rd Ave, Minneapolis, MN 55401",,"212 N 3rd Ave, Minneapolis, MN 55401","Suite A",,,,US)
26 Court Street, Boston, Massachusetts 02109, France	(26,"Court Street",,"26 Court Street",Boston,MA,02109,,FR)
10 Main Street, Toronto, ON M5V 2T6, France	(10,"Main Street",,"10 Main Street",Toronto,ON,"M5V 2T6",,FR)
\.

select id, instring, outstring as expected, parse_address(instring) as got_result
  from test_parse_address
 where instring not like '@@%' and parse_address(instring)::text != outstring;

\q

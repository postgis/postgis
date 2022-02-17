--
-- GeomFromMARC21 regression test
-- Copyright (C) 2021 University of Münster (WWU), Germany
-- Written by Jim Jones <jim.jones@uni-muenster.de>

-- ERROR: Empty string
SELECT 'empty_string',ST_GeomFromMARC21('');

-- ERROR: invalid MARC21/XML (missing <record> root element)
SELECT 'missing_root_1',ST_GeomFromMARC21('<collection></collection>');

-- ERROR: invalid MARC21/XML (unexpected root element)
SELECT 'missing_root_2',ST_GeomFromMARC21('<collection><record></record></collection>');

-- ERROR: invalid MARC21/XML
SELECT 'invalid_xml_1',ST_GeomFromMARC21('<x></x><record></record>');

-- ERROR: invalid MARC21/XML
SELECT 'invalid_xml_2',ST_GeomFromMARC21('<record></record><x></x>');

-- ERROR: Value of subfield "d" is too short. 
--        Parameter requires at least degrees with 3 digits.
SELECT 'd_too_short',
  ST_GeomFromMARC21('
	<record>
	 <datafield tag="034" ind1="0" ind2=" ">
		<subfield code="a">a</subfield>
		<subfield code="d">N42</subfield>
		<subfield code="e">W1202600</subfield>
		<subfield code="f">N0354700</subfield>
		<subfield code="g">W1204600</subfield>
	  </datafield>
	</record>');

-- ERROR: Value of subfield "e" is too short. 
--        Parameter requires at least degrees with 3 digits.
SELECT 'e_too_short',
  ST_GeomFromMARC21('
	<record>
	 <datafield tag="034" ind1="0" ind2=" ">
		<subfield code="a">a</subfield>
		<subfield code="d">W1204600</subfield>
		<subfield code="e">N42</subfield>
		<subfield code="f">N0354700</subfield>
		<subfield code="g">W1204600</subfield>
	  </datafield>
	</record>');

-- ERROR: Value of subfield "f" is too short. 
--        Parameter requires at least degrees with 3 digits.
SELECT 'f_too_short',
  ST_GeomFromMARC21('
	<record>
	 <datafield tag="034" ind1="0" ind2=" ">
		<subfield code="a">a</subfield>
		<subfield code="d">W1204600</subfield>
		<subfield code="e">W1204600</subfield>
		<subfield code="f">N42</subfield>
		<subfield code="g">W1204600</subfield>
	  </datafield>
	</record>');

-- ERROR: Value of subfield "g" is too short. 
--        Parameter requires at least degrees with 3 digits.
SELECT 'g_too_short',
  ST_GeomFromMARC21('
	<record>
	 <datafield tag="034" ind1="0" ind2=" ">
		<subfield code="a">a</subfield>
		<subfield code="d">W1204600</subfield>
		<subfield code="e">W1202600</subfield>
		<subfield code="f">N0354700</subfield>
		<subfield code="g">N42</subfield>
	  </datafield>
	</record>');

-- ERROR: Missing subfield "d". 
SELECT 'missing_d',
ST_GeomFromMARC21('
	<record>
	 <datafield tag="034" ind1="0" ind2=" ">
		<subfield code="a">a</subfield>
		<subfield code="e">W1504600</subfield>
		<subfield code="f">W1202600</subfield>
		<subfield code="g">N0354700</subfield>
	  </datafield>
	</record>');


-- ERROR: Missing subfield "e". 
SELECT 'missing_e',
  ST_GeomFromMARC21('
	<record>
	 <datafield tag="034" ind1="0" ind2=" ">
		<subfield code="a">a</subfield>
		<subfield code="d">W1504600</subfield>
		<subfield code="f">W1202600</subfield>
		<subfield code="g">N0354700</subfield>
	  </datafield>
	</record>');

-- ERROR: Missing subfield "f". 
SELECT 'missing_f', 
  ST_GeomFromMARC21('
	<record>
	 <datafield tag="034" ind1="0" ind2=" ">
		<subfield code="a">a</subfield>
		<subfield code="d">W1504600</subfield>
		<subfield code="e">W1202600</subfield>
		<subfield code="g">N0354700</subfield>
	  </datafield>
	</record>');

-- ERROR: Missing subfield "g". 
SELECT 'missing_g', 
  ST_GeomFromMARC21('
	<record>
	 <datafield tag="034" ind1="0" ind2=" ">
		<subfield code="a">a</subfield>
		<subfield code="d">W1504600</subfield>
		<subfield code="e">W1202600</subfield>
		<subfield code="f">N0354700</subfield>
	  </datafield>
	</record>');


-- ERROR: invalid cardinal direction for subfield "d"
SELECT 'invalid_hemisphere_d',
  ST_GeomFromMARC21('
	<record>
	 <datafield tag="034" ind1="0" ind2=" ">
		<subfield code="a">a</subfield>
		<subfield code="d">x1204600</subfield>
		<subfield code="e">W1202600</subfield>
		<subfield code="f">N0354700</subfield>
		<subfield code="g">N0354700</subfield>
	  </datafield>
	</record>');

-- ERROR: invalid cardinal direction for subfield "e"
SELECT 'invalid_hemisphere_e',
  ST_GeomFromMARC21('
	<record>
	 <datafield tag="034" ind1="0" ind2=" ">
		<subfield code="a">a</subfield>
		<subfield code="d">W1204600</subfield>
		<subfield code="e"> 1202600</subfield>
		<subfield code="f">N0354700</subfield>
		<subfield code="g">N0354700</subfield>
	  </datafield>
	</record>');


-- ERROR: invalid cardinal direction for subfield "f"
SELECT 'invalid_hemisphere_f',
  ST_GeomFromMARC21('
	<record>
	 <datafield tag="034" ind1="0" ind2=" ">
		<subfield code="a">a</subfield>
		<subfield code="d">W1204600</subfield>
		<subfield code="e">W1202600</subfield>
		<subfield code="f">f0354700</subfield>
		<subfield code="g">N0354700</subfield>
	  </datafield>
	</record>');

-- ERROR: invalid cardinal direction for subfield "g"
SELECT 'invalid_hemisphere_g',
  ST_GeomFromMARC21('
	<record>
	 <datafield tag="034" ind1="0" ind2=" ">
		<subfield code="a">a</subfield>
		<subfield code="d">W1204600</subfield>
		<subfield code="e">W1202600</subfield>
		<subfield code="f">N0354700</subfield>
		<subfield code="g">.0354700</subfield>
	  </datafield>
	</record>');


-- ERROR: - and W sign together in subfield "f"
SELECT '-_and_W_hemisphere',
ST_GeomFromMARC21('
	<record>
	 <datafield tag="034" ind1="0" ind2=" ">
		<subfield code="a">a</subfield>
		<subfield code="d">W1504600</subfield>
		<subfield code="e">-W1202600</subfield>
		<subfield code="f">N0354700</subfield>
		<subfield code="g">N0354700</subfield>
	  </datafield>
	</record>');

-- ERROR: - and W sign together in subfield "g"
SELECT '+_and_N_hemisphere',
ST_GeomFromMARC21('
	<record>
	 <datafield tag="034" ind1="0" ind2=" ">
		<subfield code="a">a</subfield>
		<subfield code="d">W1504600</subfield>
		<subfield code="e">W1202600</subfield>
		<subfield code="f">N+0354700</subfield>
		<subfield code="g">N0354700</subfield>
	  </datafield>
	</record>');


-- ERROR: double ++ in subfield "g"
SELECT '++_hemisphere',
ST_GeomFromMARC21('
	<record>
	 <datafield tag="034" ind1="0" ind2=" ">
		<subfield code="a">a</subfield>
		<subfield code="d">W1504600</subfield>
		<subfield code="e">W1202600</subfield>
		<subfield code="f">N0354700</subfield>
		<subfield code="g">++0354700</subfield>
	  </datafield>
	</record>');

-- ERROR: double -- in subfield "e"
SELECT '++_hemisphere',
ST_GeomFromMARC21('
	<record>
	 <datafield tag="034" ind1="0" ind2=" ">
		<subfield code="a">a</subfield>
		<subfield code="d">W1504600</subfield>
		<subfield code="e">--1202600</subfield>
		<subfield code="f">N0354700</subfield>
		<subfield code="g">0354700</subfield>
	  </datafield>
	</record>');

-- ERROR: coordinates with spaces
SELECT 'spaces_between_numbers',
ST_GeomFromMARC21('
	<record>
	 <datafield tag="034" ind1="0" ind2=" ">
		<subfield code="a">a</subfield>
		<subfield code="d">W150 4600</subfield>
		<subfield code="e">W120 2600</subfield>
		<subfield code="f">N035 4700</subfield>
		<subfield code="g">N048 4700</subfield>
	  </datafield>
	</record>');

-- ERROR: + sign betwee numbers in subfield "f"
SELECT '+_between_numbers',
ST_GeomFromMARC21('
	<record>
	 <datafield tag="034" ind1="0" ind2=" ">
		<subfield code="a">a</subfield>
		<subfield code="d">W1504600</subfield>
		<subfield code="e">W1202600</subfield>
		<subfield code="f">N035+4700</subfield>
		<subfield code="g">N0484700</subfield>
	  </datafield>
	</record>');

-- ERROR: - sign between numbers in subfield "d"
SELECT '-_between_numbers',
ST_GeomFromMARC21('
	<record>
	 <datafield tag="034" ind1="0" ind2=" ">
		<subfield code="a">a</subfield>
		<subfield code="d">W150-4600</subfield>
		<subfield code="e">W1202600</subfield>
		<subfield code="f">N0354700</subfield>
		<subfield code="g">N0484700</subfield>
	  </datafield>
	</record>');

-- ERROR: / between numbers in subfield "d"
SELECT '/_between_numbers',
ST_GeomFromMARC21('
	<record>
	 <datafield tag="034" ind1="0" ind2=" ">
		<subfield code="a">a</subfield>
		<subfield code="d">W1504600</subfield>
		<subfield code="e">W1202600</subfield>
		<subfield code="f">N0354700</subfield>
		<subfield code="g">N048/4700</subfield>
	  </datafield>
	</record>');
	
-- ERROR: * between numbers in subfield "d"
SELECT '*_between_numbers',
ST_GeomFromMARC21('
	<record>
	 <datafield tag="034" ind1="0" ind2=" ">
		<subfield code="a">a</subfield>
		<subfield code="d">W1504600</subfield>
		<subfield code="e">W1202600</subfield>
		<subfield code="f">N0354700</subfield>
		<subfield code="g">N048*4700</subfield>
	  </datafield>
	</record>');

-- ERROR: invalid value in subfield "d" (multiple decimal separators)
SELECT 'multiple_separators_d',
  ST_GeomFromMARC21('
	<record>
	 <datafield tag="034" ind1="0" ind2=" ">
		<subfield code="a">a</subfield>
		<subfield code="d">W120.46.89</subfield>
		<subfield code="e">W1202600</subfield>
		<subfield code="f">N0354700</subfield>
		<subfield code="g">W0354700</subfield>
	  </datafield>
	</record>');


-- ERROR: invalid value in subfield "e" (multiple decimal separators)
SELECT 'multiple_separators_e',
  ST_GeomFromMARC21('
	<record>
	 <datafield tag="034" ind1="0" ind2=" ">
		<subfield code="a">a</subfield>
		<subfield code="d">W1202600</subfield>
		<subfield code="e">W120.46,89</subfield>
		<subfield code="f">N0354700</subfield>
		<subfield code="g">W0354700</subfield>
	  </datafield>
	</record>');


-- ERROR: invalid value in subfield "f" (multiple decimal separators)
SELECT 'multiple_separators_f',
  ST_GeomFromMARC21('
	<record>
	 <datafield tag="034" ind1="0" ind2=" ">
		<subfield code="a">a</subfield>
		<subfield code="d">W1202600</subfield>
		<subfield code="e">N0354700</subfield>
		<subfield code="f">W120,46,89</subfield>
		<subfield code="g">W0354700</subfield>
	  </datafield>
	</record>');

-- ERROR: invalid value in subfield "g" (multiple decimal separators)
SELECT 'multiple_separators_g',
  ST_GeomFromMARC21('
	<record>
	 <datafield tag="034" ind1="0" ind2=" ">
		<subfield code="a">a</subfield>
		<subfield code="d">W1202600</subfield>
		<subfield code="e">N0354700</subfield>
		<subfield code="f">W0354700</subfield>
		<subfield code="g">W12,046.89</subfield>
	  </datafield>
	</record>');


-- ERROR: Missing value for subfield "d"
SELECT 'empty_d',
  ST_GeomFromMARC21('
  <record>
   <datafield tag="034" ind1="0" ind2=" ">
    <subfield code="a">a</subfield>
    <subfield code="d"></subfield>
    <subfield code="e">W120,2600</subfield>
    <subfield code="f">N035,4700</subfield>
    <subfield code="g">W035,4700</subfield>
    </datafield>
  </record>');

-- ERROR: Missing value for subfield "e"
SELECT 'empty_e',
  ST_GeomFromMARC21('
  <record>
   <datafield tag="034" ind1="0" ind2=" ">
    <subfield code="a">a</subfield>
    <subfield code="d">W120,2600</subfield>
    <subfield code="e"></subfield>
    <subfield code="f">N035,4700</subfield>
    <subfield code="g">W035,4700</subfield>
    </datafield>
  </record>');

-- ERROR: Missing value for subfield "f"
SELECT 'empty_f', 
  ST_GeomFromMARC21('
  <record>
   <datafield tag="034" ind1="0" ind2=" ">
    <subfield code="a">a</subfield>
    <subfield code="d">W120,2600</subfield>
    <subfield code="e">N035,4700</subfield>
    <subfield code="f"></subfield>
    <subfield code="g">W035,4700</subfield>
    </datafield>
  </record>');

-- Missing value for subfield "g"
SELECT 'empty_g', 
  ST_GeomFromMARC21('
  <record>
   <datafield tag="034" ind1="0" ind2=" ">
    <subfield code="a">a</subfield>
    <subfield code="d">W120,2600</subfield>
    <subfield code="e">N035,4700</subfield>
    <subfield code="f">N035,4700</subfield>
    <subfield code="g"></subfield>
    </datafield>
  </record>');

-- ERROR: broken XML (missing </datafield>)
SELECT 'broken_xml_1',
  ST_GeomFromMARC21('
  <record>
   <datafield tag="034" ind1="0" ind2=" ">
    <subfield code="a">a</subfield>
    <subfield code="d">W120,2600</subfield>
    <subfield code="e">N035,4700</subfield>
    <subfield code="f">N035,4700</subfield>
    <subfield code="g"></subfield>    
  </record>');

-- ERROR: broken XML (missing </record>)
SELECT 'broken_xml_2',
  ST_GeomFromMARC21('
  <record>
   <datafield tag="034" ind1="0" ind2=" ">
    <subfield code="a">a</subfield>
    <subfield code="d">W120,2600</subfield>
    <subfield code="e">N035,4700</subfield>
    <subfield code="f">N035,4700</subfield>
    <subfield code="g"></subfield>
    </datafield>');

    

-- ##############################################################
    

-- PASS: valid parameter in subfield "d" wrapped in a CDATA block
SELECT 'polygon_1',
  ST_AsText(ST_GeomFromMARC21('
  <record>
   <datafield tag="034" ind1="0" ind2=" ">
    <subfield code="a">a</subfield>
    <subfield code="d"><![CDATA[N0454700]]></subfield>
    <subfield code="e">N0354700</subfield>
    <subfield code="f">N0354700</subfield>
    <subfield code="g">N0524700</subfield>
    </datafield>
  </record>'),5);

-- PASS: valid parameter in subfield "e" wrapped in a CDATA block
SELECT 'polygon_2', 
  ST_AsText(ST_GeomFromMARC21('
  <record>
   <datafield tag="034" ind1="0" ind2=" ">
    <subfield code="a">a</subfield>
    <subfield code="d">N0454700</subfield>
    <subfield code="e"><![CDATA[N0354700]]></subfield>
    <subfield code="f">N0354700</subfield>
    <subfield code="g">N0524700</subfield>
    </datafield>
  </record>'),5);

-- PASS: valid parameter in subfield "f" wrapped in a CDATA block
SELECT 'polygon_3', 
  ST_AsText(ST_GeomFromMARC21('
  <record>
   <datafield tag="034" ind1="0" ind2=" ">
    <subfield code="a">a</subfield>
    <subfield code="d">N0454700</subfield>
    <subfield code="e">N0354700</subfield>
    <subfield code="f"><![CDATA[N0834700]]></subfield>
    <subfield code="g">N0524700</subfield>
    </datafield>
  </record>'),5);

-- PASS: valid parameter in subfield "g" wrapped in a CDATA block
SELECT 'polygon_4', 
  ST_AsText(ST_GeomFromMARC21('
  <record>
   <datafield tag="034" ind1="0" ind2=" ">
    <subfield code="a">a</subfield>
    <subfield code="d">N0454700</subfield>
    <subfield code="e">N0354700</subfield>
    <subfield code="f">N0354700</subfield>
    <subfield code="g"><![CDATA[N0524700]]></subfield>
    </datafield>
  </record>'),5);


-- PASS: valid parameters in all subfields wrapped in CDATA blocks
SELECT 'polygon_5',
  ST_AsText(ST_GeomFromMARC21('
  <record>
   <datafield tag="034" ind1="0" ind2=" ">
    <subfield code="a">a</subfield>
    <subfield code="d"><![CDATA[N0454700]]></subfield>
    <subfield code="e"><![CDATA[N0354700]]></subfield>
    <subfield code="f"><![CDATA[N0354700]]></subfield>
    <subfield code="g"><![CDATA[N0524700]]></subfield>
    </datafield>
  </record>'),5);


-- PASS: NULL parameter
SELECT 'null_parameter',ST_GeomFromMARC21(NULL);

-- PASS: MARC21/XML record without a datafield:034 element
SELECT 'empty_record',ST_GeomFromMARC21('<record></record>');

-- PASS: Datafield 034 without coordinates (subfields "d","e","f" and "g")
SELECT '034_without_geo', ST_GeomFromMARC21('
  <record>
   <datafield tag="034" ind1="0" ind2=" "></datafield>
  </record>');
  
  
-- PASS: coordinates format "hdddmmss"
SELECT 'polygon_6',
  ST_AsText(ST_GeomFromMARC21('
  <record xmlns="http://www.loc.gov/MARC21/slim">
    <leader>01643cem a2200373 a 4500</leader>
    <controlfield tag="001">  2006635698</controlfield>
    <datafield tag="034" ind1="1" ind2=" ">
      <subfield code="a">a</subfield>
      <subfield code="d">E0073300</subfield>
      <subfield code="e">E0074300</subfield>
      <subfield code="f">N0520100</subfield>
      <subfield code="g">N0515700</subfield>
    </datafield>
  </record>'),5);

-- PASS: coordinates formats [WE]dddmmss, +dddmmss, dddmmss
SELECT 'polygon_7',
  ST_AsText(ST_GeomFromMARC21('
  <record xmlns="http://www.loc.gov/MARC21/slim">
    <datafield tag="034" ind1="1" ind2=" ">
      <subfield code="a">a</subfield>
	  <subfield code="d">E0073300</subfield>
	  <subfield code="e">+0074300</subfield>
	  <subfield code="f">0520100</subfield>
	  <subfield code="g">N0515700</subfield>
	</datafield>
  </record>'),5);

-- PASS: coordinates formats [WS]dddmmss'
SELECT 'polygon_8',
  ST_AsText(ST_GeomFromMARC21('
  <record xmlns="http://www.loc.gov/MARC21/slim">
    <datafield tag="034" ind1="1" ind2=" ">
	  <subfield code="a">a</subfield>
	  <subfield code="d">W0584745</subfield>
	  <subfield code="e">W0582451</subfield>
	  <subfield code="f">S0513416</subfield>
	  <subfield code="g">S0514450</subfield>
	</datafield>
  </record>'),5);
  
-- PASS: coordinates formats [WS]dddmmss, -dddmmss
SELECT 'polygon_9',
  ST_AsText(ST_GeomFromMARC21('
  <record xmlns="http://www.loc.gov/MARC21/slim">
    <datafield tag="034" ind1="1" ind2=" ">
	  <subfield code="a">a</subfield>
	  <subfield code="d">-0584745</subfield>
	  <subfield code="e">W0582451</subfield>
	  <subfield code="f">S0513416</subfield>
	  <subfield code="g">-0514450</subfield>
	</datafield>
  </record>'),5);  
  
-- PASS: coordinates format [EN]ddd.dddddd 
SELECT 'polygon_10',
  ST_AsText(ST_GeomFromMARC21('
  <record xmlns="http://www.loc.gov/MARC21/slim">
    <datafield tag="034" ind1="1" ind2=" ">
	  <subfield code="a">a</subfield>
	  <subfield code="d">E002.318055</subfield>
	  <subfield code="e">E004.484444</subfield>
	  <subfield code="f">N051.195833</subfield>
	  <subfield code="g">N049.618333</subfield>
	</datafield>
  </record>'),5);   

-- PASS: coordinates format [EN]ddd,dddddd
SELECT 'polygon_11',
  ST_AsText(ST_GeomFromMARC21('
  <record xmlns="http://www.loc.gov/MARC21/slim">
    <datafield tag="034" ind1="1" ind2=" ">
	  <subfield code="a">a</subfield>
	  <subfield code="d">E002,318055</subfield>
	  <subfield code="e">E004,484444</subfield>
	  <subfield code="f">N051,195833</subfield>
	  <subfield code="g">N049,618333</subfield>
	</datafield>
  </record>'),5);

-- PASS: coordinates formats [EN]ddd.dddddd, +ddd.dddddd, ddd.dddddd
SELECT 'polygon_13',
  ST_AsText(ST_GeomFromMARC21('
  <record xmlns="http://www.loc.gov/MARC21/slim">
    <datafield tag="034" ind1="1" ind2=" ">
	  <subfield code="a">a</subfield>
	  <subfield code="d">E002.318055</subfield>
	  <subfield code="e">+004.484444</subfield>
	  <subfield code="f">051.195833</subfield>
	  <subfield code="g">N049.618333</subfield>
	</datafield>
  </record>'),5);    

-- PASS: coordinates formats [EN]dddmm.mmmmm, +dddmm.mmmmm, dddmm.mmmmm
SELECT 'polygon_14',
  ST_AsText(ST_GeomFromMARC21('
  <record xmlns="http://www.loc.gov/MARC21/slim">
    <datafield tag="034" ind1="1" ind2=" ">
	  <subfield code="a">a</subfield>
	  <subfield code="d">E00231.8055</subfield>
	  <subfield code="e">+00448.4444</subfield>
	  <subfield code="f">05119.5833</subfield>
	  <subfield code="g">N04961.8333</subfield>
	</datafield>
  </record>'),5);      
  
-- PASS: coordinates formats [EN]dddmmss.sssss, +dddmmss.sssss, dddmmss.sssss
SELECT 'polygon_15',
  ST_AsText(ST_GeomFromMARC21('
  <record xmlns="http://www.loc.gov/MARC21/slim">
    <datafield tag="034" ind1="1" ind2=" ">
	  <subfield code="a">a</subfield>
	  <subfield code="d">E0023180.55</subfield>
	  <subfield code="e">+0044844.44</subfield>
	  <subfield code="f">0511958.33</subfield>
	  <subfield code="g">N0496183.33</subfield>
	</datafield>
  </record>'),5);      

-- PASS: very large number encoded as hdddmmss+
SELECT 'polygon_16',
ST_AsText(ST_GeomFromMARC21('
	<record>
	 <datafield tag="034" ind1="0" ind2=" ">
		<subfield code="a">a</subfield>
		<subfield code="d">W1542654165434046468468471164764064646468713100465189461014404600</subfield>
		<subfield code="e">W1202600165434046468468471164764064646468713100465189461014404600</subfield>
		<subfield code="f">N0354700165434046468468471164764064646468713100465189461014404600</subfield>
		<subfield code="g">N0484700165434046468468471164764064646468713100465189461014404600</subfield>
	  </datafield>
	</record>'),5);

-- PASS: very large number encoded with decimal degrees
SELECT 'polygon_17',
ST_AsText(ST_GeomFromMARC21('
	<record>
	 <datafield tag="034" ind1="0" ind2=" ">
		<subfield code="a">a</subfield>
		<subfield code="d">W154.2654165434046468468471164764064646468713100465189461014404600</subfield>
		<subfield code="e">W120.2600165434046468468471164764064646468713100465189461014404600</subfield>
		<subfield code="f">N035.4700165434046468468471164764064646468713100465189461014404600</subfield>
		<subfield code="g">N048.4700165434046468468471164764064646468713100465189461014404600</subfield>
	  </datafield>
	</record>'),5);

-- PASS: very large number encoded with decimal minutes
SELECT 'polygon_18',
ST_AsText(ST_GeomFromMARC21('
	<record>
	 <datafield tag="034" ind1="0" ind2=" ">
		<subfield code="a">a</subfield>
		<subfield code="d">W15426.54165434046468468471164764064646468713100465189461014404600</subfield>
		<subfield code="e">W12026.00165434046468468471164764064646468713100465189461014404600</subfield>
		<subfield code="f">N03547.00165434046468468471164764064646468713100465189461014404600</subfield>
		<subfield code="g">N04847.00165434046468468471164764064646468713100465189461014404600</subfield>
	  </datafield>
	</record>'),5);

-- PASS: very large number encoded with decimal minutes
SELECT 'polygon_19',
ST_AsText(ST_GeomFromMARC21('
	<record>
	 <datafield tag="034" ind1="0" ind2=" ">
		<subfield code="a">a</subfield>
		<subfield code="d">W1542654165434046468468471164764064646468713100465189461014404600</subfield>
		<subfield code="e">W1202600.165434046468468471164764064646468713100465189461014404600</subfield>
		<subfield code="f">N0354700.165434046468468471164764064646468713100465189461014404600</subfield>
		<subfield code="g">N0484700.165434046468468471164764064646468713100465189461014404600</subfield>
	  </datafield>
	</record>'),5);


-- PASS: coordinates format [EN]ddd.ddddd
SELECT 'point_1',
  ST_AsText(ST_GeomFromMARC21('
  <record xmlns="http://www.loc.gov/MARC21/slim">
    <leader>01643cem a2200373 a 4500</leader>
    <controlfield tag="001">  2006635698</controlfield>
    <datafield tag="034" ind1="1" ind2=" ">
      <subfield code="a">a</subfield>
      <subfield code="d">E013.416669</subfield>
      <subfield code="e">E013.416669</subfield>
      <subfield code="f">N052.500000</subfield>
      <subfield code="g">N052.500000</subfield>
    </datafield>
  </record>'),5);     

-- PASS: coordinates format [EN]dddmm.mmmmm
SELECT 'point_2',
  ST_AsText(ST_GeomFromMARC21('
  <record xmlns="http://www.loc.gov/MARC21/slim">
    <leader>01643cem a2200373 a 4500</leader>
    <controlfield tag="001">  2006635698</controlfield>
    <datafield tag="034" ind1="1" ind2=" ">
      <subfield code="a">a</subfield>
      <subfield code="d">E01341.6669</subfield>
      <subfield code="e">E01341.6669</subfield>
      <subfield code="f">N05250.0000</subfield>
      <subfield code="g">N05250.0000</subfield>
    </datafield>
  </record>'),5);     

-- PASS: coordinates format [EN]dddmmss.sssss
SELECT 'point_3',
  ST_AsText(ST_GeomFromMARC21('
  <record xmlns="http://www.loc.gov/MARC21/slim">
    <leader>01643cem a2200373 a 4500</leader>
    <controlfield tag="001">  2006635698</controlfield>
    <datafield tag="034" ind1="1" ind2=" ">
      <subfield code="a">a</subfield>
      <subfield code="d">E0134166.69</subfield>
      <subfield code="e">E0134166.69</subfield>
      <subfield code="f">N0525000.00</subfield>
      <subfield code="g">N0525000.00</subfield>
    </datafield>
  </record>'),5);  
  
-- PASS: coordinates formats [EN]dddmmss.sssss, +dddmmss.sssss, dddmmss.sssss  
SELECT 'multipolygon_1',
  ST_AsText(ST_GeomFromMARC21('
  <record xmlns="http://www.loc.gov/MARC21/slim">
    <leader>01643cem a2200373 a 4500</leader>
    <controlfield tag="001">  2006635698</controlfield>
    <datafield tag="034" ind1="1" ind2=" ">
      <subfield code="a">a</subfield>
      <subfield code="d">E0073300</subfield>
      <subfield code="e">E0074300</subfield>
      <subfield code="f">N0520100</subfield>
      <subfield code="g">N0515700</subfield>
    </datafield>
    <datafield tag="034" ind1="1" ind2=" ">
	  <subfield code="a">a</subfield>
	  <subfield code="d">W0584745</subfield>
	  <subfield code="e">W0582451</subfield>
	  <subfield code="f">S0513416</subfield>
	  <subfield code="g">S0514450</subfield>
	</datafield>
  </record>'),5);      

-- PASS: coordinates formats [ENW]ddd.ddddd
SELECT 'multipoint_1',
  ST_AsText(ST_GeomFromMARC21('
  <record xmlns="http://www.loc.gov/MARC21/slim">
    <leader>01643cem a2200373 a 4500</leader>
    <controlfield tag="001">  2006635698</controlfield>
    <datafield tag="034" ind1="1" ind2=" ">
      <subfield code="a">a</subfield>
      <subfield code="d">E013.416669</subfield>
      <subfield code="e">E013.416669</subfield>
      <subfield code="f">N052.500000</subfield>
      <subfield code="g">N052.500000</subfield>
    </datafield>
    <datafield tag="034" ind1="1" ind2=" ">
	  <subfield code="a">a</subfield>
	  <subfield code="d">W000.125740</subfield>
	  <subfield code="e">W000.125740</subfield>
	  <subfield code="f">N051.508530</subfield>
	  <subfield code="g">N051.508530</subfield>
	</datafield>
  </record>'),5);      

-- PASS: coordinates formats [ENW]ddd.ddddd
SELECT 'geocollection_1',
  ST_AsText(ST_GeomFromMARC21('
  <record xmlns="http://www.loc.gov/MARC21/slim">
    <leader>01643cem a2200373 a 4500</leader>
    <controlfield tag="001">  2006635698</controlfield>
    <datafield tag="034" ind1="1" ind2=" ">
      <subfield code="a">a</subfield>
      <subfield code="d">E013.416669</subfield>
      <subfield code="e">E013.416669</subfield>
      <subfield code="f">N052.500000</subfield>
      <subfield code="g">N052.500000</subfield>
    </datafield>
    <datafield tag="034" ind1="1" ind2=" ">
	  <subfield code="a">a</subfield>
	  <subfield code="d">W0584745</subfield>
	  <subfield code="e">W0582451</subfield>
	  <subfield code="f">S0513416</subfield>
	  <subfield code="g">S0514450</subfield>
    </datafield>
  </record>'),5);      

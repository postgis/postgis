
/**
 * Creates a random unicode string containing characters in a given range 
 */

CREATE OR REPLACE FUNCTION public.random_unicode_string(low integer,high integer)
    RETURNS text
    LANGUAGE 'plpgsql'
    COST 100
    VOLATILE STRICT PARALLEL UNSAFE
AS $$
DECLARE
  res text := '';
  char_num int := 1;
  min_surrogate int := 55296;
  max_surrogate int := 57343;
  max_unicode int := 1111998;
BEGIN
   FOR i IN 1 .. 8 LOOP
     char_num := floor(random()* (high-low + 1) + low);
	 IF (char_num > 0 AND char_num < min_surrogate) OR
	    (char_num > max_surrogate AND char_num <= max_unicode) THEN
       res := res || chr(char_num);
	 END IF;
   END LOOP;

   RETURN res;
END;
$$;

/**
 * ## Stress test for ST_GeomFromMARC21 ##
 *
 * This test creates invalid MARC21/XML datafields with all unicode chars. 
 * It iterates over all planes and their available char values, firing 
 * over 1.1m ST_GeomFromMARC21 requests with invalid characters.    
*/

DO $$
DECLARE
  utf8_string text := '';
  min_surrogate int := 55296;
  max_surrogate int := 57343;
  num_chars_plane int := 65536;
  num_planes int := 17;
  max_unicode int := 1111998;

  marc_string text;
  nt int := 0;
BEGIN

  FOR j IN 1 .. num_planes LOOP

    FOR i IN (j*num_chars_plane)-num_chars_plane .. j*num_chars_plane LOOP

      IF i > 0 AND i < min_surrogate OR i > max_surrogate AND i <= max_unicode THEN
        utf8_string := utf8_string || chr(i);
      END IF;

	  marc_string := format('
		<record>
		  <datafield tag="034" ind1="0" ind2=" ">
			<subfield code="a">a</subfield>
			<subfield code="d">%s</subfield>
			<subfield code="e">%s</subfield>
			<subfield code="f">%s</subfield>
			<subfield code="g">%s</subfield>
		  </datafield>
		</record>',
		random_unicode_string((j*num_chars_plane)-num_chars_plane,j*num_chars_plane),
		random_unicode_string((j*num_chars_plane)-num_chars_plane,j*num_chars_plane),
		random_unicode_string((j*num_chars_plane)-num_chars_plane,j*num_chars_plane),
		random_unicode_string((j*num_chars_plane)-num_chars_plane,j*num_chars_plane));

	  --RAISE INFO '%s',marc_string;
	  BEGIN
	    SELECT ST_GeomFromMARC21(marc_string);
	    EXCEPTION WHEN others THEN nt:=nt+1;
	  END;

	END LOOP;

	  --RAISE INFO 'ST_GeomFromMARC21 fired % times with random unicode chars between % and %. ',
	  --             nt,(j*num_chars_plane)-num_chars_plane,j*num_chars_plane;
	  nt := 0;
	BEGIN

	  SELECT ST_GeomFromMARC21(utf8_string);
      EXCEPTION WHEN others THEN
         --RAISE INFO 'ST_GeomFromMARC21 fired once with all chars from plane % (chars from % to %). Function returns error message -> "%"',
         --            j,(j*num_chars_plane)-num_chars_plane,j*num_chars_plane,SQLERRM;

	END;
	utf8_string := '';


  END LOOP;

END;$$;


 
 

/**
 * Valid MARC21/XML documents encoded as degrees/minutes/seconds
 *  
 */

WITH j (gid,doc,area) AS (
 VALUES
  (1,	'<record xmlns="http://www.loc.gov/MARC21/slim">
  <leader>01643cem a2200373 a 4500</leader>
  <controlfield tag="001">  2006635698</controlfield>
  <datafield tag="034" ind1="1" ind2=" ">
  <subfield code="a">a</subfield>
  <subfield code="b">10000</subfield>
  <subfield code="d">E0073300</subfield>
  <subfield code="e">E0074300</subfield>
  <subfield code="f">N0520100</subfield>
  <subfield code="g">N0515700</subfield>
  </datafield>
  <datafield tag="040" ind1=" " ind2=" ">
  <subfield code="a">DLC</subfield>
  <subfield code="c">DLC</subfield>
  <subfield code="d">DLC</subfield>
  </datafield>
  <datafield tag="045" ind1="0" ind2=" ">
  <subfield code="b">d1972</subfield>
  </datafield>
  <datafield tag="050" ind1="0" ind2="0">
  <subfield code="a">G6299.M787 1972</subfield>
  <subfield code="b">.S6</subfield>
  </datafield>
  <datafield tag="052" ind1=" " ind2=" ">
  <subfield code="a">6299</subfield>
  <subfield code="b">M787</subfield>
  </datafield>
  <datafield tag="110" ind1="1" ind2=" ">
  <subfield code="a">Soviet Union.</subfield>
  <subfield code="b">Sovetskai︠a︡ Armii︠a︡.</subfield>
  <subfield code="b">Generalʹnyĭ shtab.</subfield>
  </datafield>
  <datafield tag="245" ind1="1" ind2="0">
  <subfield code="a">Mi︠u︡nster (M-32-4, N-32-136) :</subfield>
  <subfield code="b">FRG, zemli︠a︡ Severnyĭ Reĭn-Vestfalii︠a︡ /</subfield>
  <subfield code="c">Generalʹnyĭ shtab.</subfield>
  </datafield>
  <datafield tag="250" ind1=" " ind2=" ">
  <subfield code="a">Izd. 1979 g.</subfield>
  </datafield>
  <datafield tag="255" ind1=" " ind2=" ">
  <subfield code="a">Scale 1:10,000. 1 cm. to 100 m.</subfield>
  <subfield code="c">(E 7⁰33ʹ--E 7⁰43ʹ/N 52⁰01ʹ--N 51⁰57ʹ).</subfield>
  </datafield>
  <datafield tag="260" ind1=" " ind2=" ">
  <subfield code="a">[Moscow?] :</subfield>
  <subfield code="b">Generalʹnyĭ shtab,</subfield>
  <subfield code="c">1979.</subfield>
  </datafield>
  <datafield tag="300" ind1=" " ind2=" ">
  <subfield code="a">1 map on 2 sheets :</subfield>
  <subfield code="b">col. ;</subfield>
  <subfield code="c">111 x 100 cm., sheets 106 x 83 cm.</subfield>
  </datafield>
  <datafield tag="500" ind1=" " ind2=" ">
  <subfield code="a">Military topographic map showing numbered government, military, transportation, and industrial facilities.</subfield>
  </datafield>
  <datafield tag="500" ind1=" " ind2=" ">
  <subfield code="a">Relief shown by contours and spot heights.</subfield>
  </datafield>
  <datafield tag="500" ind1=" " ind2=" ">
  <subfield code="a">"Sostavleno ... s ispolʹzovaniem materialov na 1969,71,72 gg."</subfield>
  </datafield>
  <datafield tag="546" ind1=" " ind2=" ">
  <subfield code="a">In Russian.</subfield>
  </datafield>
  <datafield tag="500" ind1=" " ind2=" ">
  <subfield code="a">"Sistema koordinat 1942 g."</subfield>
  </datafield>
  <datafield tag="500" ind1=" " ind2=" ">
  <subfield code="a">Former security classification in margins: Sekretno.</subfield>
  </datafield>
  <datafield tag="500" ind1=" " ind2=" ">
  <subfield code="a">Sheets numbered: List 1 [northern] -- List 2 [southern].</subfield>
  </datafield>
  <datafield tag="500" ind1=" " ind2=" ">
  <subfield code="a">Includes text, indexes, and sheet-assembly diagrams.</subfield>
  </datafield>
  <datafield tag="500" ind1=" " ind2=" ">
  <subfield code="a">"I-2/1 XI-79-K. I-2/2 XI 79-K."</subfield>
  </datafield>
  <datafield tag="651" ind1=" " ind2="0">
  <subfield code="a">Münster in Westfalen (Germany)</subfield>
  <subfield code="v">Maps, Topographic.</subfield>
  </datafield>
  <datafield tag="651" ind1=" " ind2="0">
  <subfield code="a">Münster in Westfalen Metropolitan Area (Germany)</subfield>
  <subfield code="v">Maps, Topographic.</subfield>
  </datafield>
  <datafield tag="650" ind1=" " ind2="0">
  <subfield code="a">Military maps.</subfield>
  </datafield>
  </record>', 0.0111111111111105),
  (2,	'<record xmlns="http://www.loc.gov/MARC21/slim">
  <leader>01643cem a2200373 a 4500</leader>
  <controlfield tag="001">  2006635698</controlfield>
  <controlfield tag="003">DLC</controlfield>
  <controlfield tag="005">20100605083518.0</controlfield>
  <controlfield tag="007">aj|canzn</controlfield>
  <controlfield tag="008">061214s1979    ru ag     a  f  1   ruso </controlfield>
  <datafield tag="010" ind1=" " ind2=" ">
  <subfield code="a">  2006635698</subfield>
  </datafield>
  <datafield tag="034" ind1="1" ind2=" ">
  <subfield code="a">a</subfield>
  <subfield code="b">10000</subfield>
  <subfield code="d">E0073300</subfield>
  <subfield code="e">+0074300</subfield>
  <subfield code="f">0520100</subfield>
  <subfield code="g">N0515700</subfield>
  </datafield>
  <datafield tag="040" ind1=" " ind2=" ">
  <subfield code="a">DLC</subfield>
  <subfield code="c">DLC</subfield>
  <subfield code="d">DLC</subfield>
  </datafield>
  <datafield tag="045" ind1="0" ind2=" ">
  <subfield code="b">d1972</subfield>
  </datafield>
  <datafield tag="050" ind1="0" ind2="0">
  <subfield code="a">G6299.M787 1972</subfield>
  <subfield code="b">.S6</subfield>
  </datafield>
  <datafield tag="052" ind1=" " ind2=" ">
  <subfield code="a">6299</subfield>
  <subfield code="b">M787</subfield>
  </datafield>
  <datafield tag="110" ind1="1" ind2=" ">
  <subfield code="a">Soviet Union.</subfield>
  <subfield code="b">Sovetskai︠a︡ Armii︠a︡.</subfield>
  <subfield code="b">Generalʹnyĭ shtab.</subfield>
  </datafield>
  <datafield tag="245" ind1="1" ind2="0">
  <subfield code="a">Mi︠u︡nster (M-32-4, N-32-136) :</subfield>
  <subfield code="b">FRG, zemli︠a︡ Severnyĭ Reĭn-Vestfalii︠a︡ /</subfield>
  <subfield code="c">Generalʹnyĭ shtab.</subfield>
  </datafield>
  <datafield tag="250" ind1=" " ind2=" ">
  <subfield code="a">Izd. 1979 g.</subfield>
  </datafield>
  <datafield tag="255" ind1=" " ind2=" ">
  <subfield code="a">Scale 1:10,000. 1 cm. to 100 m.</subfield>
  <subfield code="c">(E 7⁰33ʹ--E 7⁰43ʹ/N 52⁰01ʹ--N 51⁰57ʹ).</subfield>
  </datafield>
  <datafield tag="260" ind1=" " ind2=" ">
  <subfield code="a">[Moscow?] :</subfield>
  <subfield code="b">Generalʹnyĭ shtab,</subfield>
  <subfield code="c">1979.</subfield>
  </datafield>
  <datafield tag="300" ind1=" " ind2=" ">
  <subfield code="a">1 map on 2 sheets :</subfield>
  <subfield code="b">col. ;</subfield>
  <subfield code="c">111 x 100 cm., sheets 106 x 83 cm.</subfield>
  </datafield>
  <datafield tag="500" ind1=" " ind2=" ">
  <subfield code="a">Military topographic map showing numbered government, military, transportation, and industrial facilities.</subfield>
  </datafield>
  <datafield tag="500" ind1=" " ind2=" ">
  <subfield code="a">Relief shown by contours and spot heights.</subfield>
  </datafield>
  <datafield tag="500" ind1=" " ind2=" ">
  <subfield code="a">"Sostavleno ... s ispolʹzovaniem materialov na 1969,71,72 gg."</subfield>
  </datafield>
  <datafield tag="546" ind1=" " ind2=" ">
  <subfield code="a">In Russian.</subfield>
  </datafield>
  <datafield tag="500" ind1=" " ind2=" ">
  <subfield code="a">"Sistema koordinat 1942 g."</subfield>
  </datafield>
  <datafield tag="500" ind1=" " ind2=" ">
  <subfield code="a">Former security classification in margins: Sekretno.</subfield>
  </datafield>
  <datafield tag="500" ind1=" " ind2=" ">
  <subfield code="a">Sheets numbered: List 1 [northern] -- List 2 [southern].</subfield>
  </datafield>
  <datafield tag="500" ind1=" " ind2=" ">
  <subfield code="a">Includes text, indexes, and sheet-assembly diagrams.</subfield>
  </datafield>
  <datafield tag="500" ind1=" " ind2=" ">
  <subfield code="a">"I-2/1 XI-79-K. I-2/2 XI 79-K."</subfield>
  </datafield>
  <datafield tag="651" ind1=" " ind2="0">
  <subfield code="a">Münster in Westfalen (Germany)</subfield>
  <subfield code="v">Maps, Topographic.</subfield>
  </datafield>
  <datafield tag="651" ind1=" " ind2="0">
  <subfield code="a">Münster in Westfalen Metropolitan Area (Germany)</subfield>
  <subfield code="v">Maps, Topographic.</subfield>
  </datafield>
  <datafield tag="650" ind1=" " ind2="0">
  <subfield code="a">Military maps.</subfield>
  </datafield>
  </record>', 0.0111111111111105),
  (3,'<record xmlns="http://www.loc.gov/MARC21/slim">
  <leader>01374cem a22003614a 4500</leader>
  <controlfield tag="001">  2005630780</controlfield>
  <controlfield tag="003">DLC</controlfield>
  <controlfield tag="005">20050630152517.0</controlfield>
  <controlfield tag="007">aj canzn</controlfield>
  <controlfield tag="008">050630s2001    enka   bh a  f  0   eng  </controlfield>
  <datafield tag="010" ind1=" " ind2=" ">
  <subfield code="a">  2005630780</subfield>
  </datafield>
  <datafield tag="034" ind1="1" ind2=" ">
  <subfield code="a">a</subfield>
  <subfield code="b">25000</subfield>
  <subfield code="d">W0584745</subfield>
  <subfield code="e">W0582451</subfield>
  <subfield code="f">S0513416</subfield>
  <subfield code="g">S0514450</subfield>
  </datafield>
  <datafield tag="040" ind1=" " ind2=" ">
  <subfield code="a">DLC</subfield>
  <subfield code="c">DLC</subfield>
  </datafield>
  <datafield tag="042" ind1=" " ind2=" ">
  <subfield code="a">pcc</subfield>
  </datafield>
  <datafield tag="045" ind1="0" ind2=" ">
  <subfield code="b">d2000</subfield>
  </datafield>
  <datafield tag="050" ind1="0" ind2="0">
  <subfield code="a">G9177.O5 2000</subfield>
  <subfield code="b">.G7</subfield>
  </datafield>
  <datafield tag="052" ind1=" " ind2=" ">
  <subfield code="a">9177</subfield>
  <subfield code="b">O5</subfield>
  </datafield>
  <datafield tag="110" ind1="1" ind2=" ">
  <subfield code="a">Great Britain.</subfield>
  <subfield code="b">Defence Geographic and Imagery Intelligence Agency.</subfield>
  </datafield>
  <datafield tag="245" ind1="1" ind2="0">
  <subfield code="a">Falkland Island training areas 1:25,000.</subfield>
  <subfield code="p">Onion Range /</subfield>
  <subfield code="c">produced by DGIA, Ministry of Defence, United Kingdom, 2001.</subfield>
  </datafield>
  <datafield tag="246" ind1="3" ind2="0">
  <subfield code="a">Onion Range</subfield>
  </datafield>
  <datafield tag="250" ind1=" " ind2=" ">
  <subfield code="a">Ed. 4-GSGS.</subfield>
  </datafield>
  <datafield tag="255" ind1=" " ind2=" ">
  <subfield code="a">Scale 1:25,000 ;</subfield>
  <subfield code="b">transverse Mercator proj.</subfield>
  <subfield code="c">(W 58⁰47ʹ45ʺ--W 58⁰24ʹ51ʺ/S 51⁰34ʹ16ʺ--S 51⁰44ʹ50ʺ).</subfield>
  </datafield>
  <datafield tag="260" ind1=" " ind2=" ">
  <subfield code="a">London :</subfield>
  <subfield code="b">DGIA, Ministry of Defence,</subfield>
  <subfield code="c">c2001.</subfield>
  </datafield>
  <datafield tag="300" ind1=" " ind2=" ">
  <subfield code="a">1 map :</subfield>
  <subfield code="b">col. ;</subfield>
  <subfield code="c">77 x 104 cm.</subfield>
  </datafield>
  <datafield tag="490" ind1="1" ind2=" ">
  <subfield code="a">Series GSGS ;</subfield>
  <subfield code="v">5616</subfield>
  </datafield>
  <datafield tag="500" ind1=" " ind2=" ">
  <subfield code="a">Shows region of Onion Camp.</subfield>
  </datafield>
  <datafield tag="500" ind1=" " ind2=" ">
  <subfield code="a">"Overprint information supplied May 2000."</subfield>
  </datafield>
  <datafield tag="500" ind1=" " ind2=" ">
  <subfield code="a">Relief shown by contours.</subfield>
  </datafield>
  <datafield tag="500" ind1=" " ind2=" ">
  <subfield code="a">In lower right margin: Onion Range.</subfield>
  </datafield>
  <datafield tag="500" ind1=" " ind2=" ">
  <subfield code="a">"UTM grid/Sapper Hill 1943 datum."</subfield>
  </datafield>
  <datafield tag="500" ind1=" " ind2=" ">
  <subfield code="a">"Warning: Air information is not depicted on this map. ..."</subfield>
  </datafield>
  <datafield tag="651" ind1=" " ind2="0">
  <subfield code="a">Onion Camp Region (Falkland Islands)</subfield>
  <subfield code="v">Maps, Topographic.</subfield>
  </datafield>
  <datafield tag="830" ind1=" " ind2="0">
  <subfield code="a">GSGS (Series) ;</subfield>
  <subfield code="v">5616.</subfield>
  </datafield>
  </record>', 0.06721574074073855),
  (4,'<record xmlns="http://www.loc.gov/MARC21/slim">
  <leader>01374cem a22003614a 4500</leader>
  <controlfield tag="001">  2005630780</controlfield>
  <controlfield tag="003">DLC</controlfield>
  <controlfield tag="005">20050630152517.0</controlfield>
  <controlfield tag="007">aj canzn</controlfield>
  <controlfield tag="008">050630s2001    enka   bh a  f  0   eng  </controlfield>
  <datafield tag="010" ind1=" " ind2=" ">
  <subfield code="a">  2005630780</subfield>
  </datafield>
  <datafield tag="034" ind1="1" ind2=" ">
  <subfield code="a">a</subfield>
  <subfield code="b">25000</subfield>
  <subfield code="d">-0584745</subfield>
  <subfield code="e">W0582451</subfield>
  <subfield code="f">S0513416</subfield>
  <subfield code="g">-0514450</subfield>
  </datafield>
  <datafield tag="040" ind1=" " ind2=" ">
  <subfield code="a">DLC</subfield>
  <subfield code="c">DLC</subfield>
  </datafield>
  <datafield tag="042" ind1=" " ind2=" ">
  <subfield code="a">pcc</subfield>
  </datafield>
  <datafield tag="045" ind1="0" ind2=" ">
  <subfield code="b">d2000</subfield>
  </datafield>
  <datafield tag="050" ind1="0" ind2="0">
  <subfield code="a">G9177.O5 2000</subfield>
  <subfield code="b">.G7</subfield>
  </datafield>
  <datafield tag="052" ind1=" " ind2=" ">
  <subfield code="a">9177</subfield>
  <subfield code="b">O5</subfield>
  </datafield>
  <datafield tag="110" ind1="1" ind2=" ">
  <subfield code="a">Great Britain.</subfield>
  <subfield code="b">Defence Geographic and Imagery Intelligence Agency.</subfield>
  </datafield>
  <datafield tag="245" ind1="1" ind2="0">
  <subfield code="a">Falkland Island training areas 1:25,000.</subfield>
  <subfield code="p">Onion Range /</subfield>
  <subfield code="c">produced by DGIA, Ministry of Defence, United Kingdom, 2001.</subfield>
  </datafield>
  <datafield tag="246" ind1="3" ind2="0">
  <subfield code="a">Onion Range</subfield>
  </datafield>
  <datafield tag="250" ind1=" " ind2=" ">
  <subfield code="a">Ed. 4-GSGS.</subfield>
  </datafield>
  <datafield tag="255" ind1=" " ind2=" ">
  <subfield code="a">Scale 1:25,000 ;</subfield>
  <subfield code="b">transverse Mercator proj.</subfield>
  <subfield code="c">(W 58⁰47ʹ45ʺ--W 58⁰24ʹ51ʺ/S 51⁰34ʹ16ʺ--S 51⁰44ʹ50ʺ).</subfield>
  </datafield>
  <datafield tag="260" ind1=" " ind2=" ">
  <subfield code="a">London :</subfield>
  <subfield code="b">DGIA, Ministry of Defence,</subfield>
  <subfield code="c">c2001.</subfield>
  </datafield>
  <datafield tag="300" ind1=" " ind2=" ">
  <subfield code="a">1 map :</subfield>
  <subfield code="b">col. ;</subfield>
  <subfield code="c">77 x 104 cm.</subfield>
  </datafield>
  <datafield tag="490" ind1="1" ind2=" ">
  <subfield code="a">Series GSGS ;</subfield>
  <subfield code="v">5616</subfield>
  </datafield>
  <datafield tag="500" ind1=" " ind2=" ">
  <subfield code="a">Shows region of Onion Camp.</subfield>
  </datafield>
  <datafield tag="500" ind1=" " ind2=" ">
  <subfield code="a">"Overprint information supplied May 2000."</subfield>
  </datafield>
  <datafield tag="500" ind1=" " ind2=" ">
  <subfield code="a">Relief shown by contours.</subfield>
  </datafield>
  <datafield tag="500" ind1=" " ind2=" ">
  <subfield code="a">In lower right margin: Onion Range.</subfield>
  </datafield>
  <datafield tag="500" ind1=" " ind2=" ">
  <subfield code="a">"UTM grid/Sapper Hill 1943 datum."</subfield>
  </datafield>
  <datafield tag="500" ind1=" " ind2=" ">
  <subfield code="a">"Warning: Air information is not depicted on this map. ..."</subfield>
  </datafield>
  <datafield tag="651" ind1=" " ind2="0">
  <subfield code="a">Onion Camp Region (Falkland Islands)</subfield>
  <subfield code="v">Maps, Topographic.</subfield>
  </datafield>
  <datafield tag="830" ind1=" " ind2="0">
  <subfield code="a">GSGS (Series) ;</subfield>
  <subfield code="v">5616.</subfield>
  </datafield>
  </record>', 0.06721574074073855),

  (5,'
  <record xmlns="http://www.loc.gov/MARC21/slim">
  <datafield tag="034" ind1="0" ind2=" ">
  <subfield code="a">a</subfield>
  <subfield code="d">E002.318055</subfield>
  <subfield code="e">E004.484444</subfield>
  <subfield code="f">N051.195833</subfield>
  <subfield code="g">N049.618333</subfield>
  <subfield code="9">A:dcx</subfield>
  </datafield>
  <datafield tag="245" ind1="1" ind2="0">
  <subfield code="a">Final British Offensive, August-November, 1918</subfield>
  </datafield>
  </record>',3.4174786475000007),
  (6,'
  <record xmlns="http://www.loc.gov/MARC21/slim">
  <datafield tag="034" ind1="0" ind2=" ">
  <subfield code="a">a</subfield>
  <subfield code="d">E002,318055</subfield>
  <subfield code="e">E004,484444</subfield>
  <subfield code="f">N051,195833</subfield>
  <subfield code="g">N049,618333</subfield>
  <subfield code="9">A:dcx</subfield>
  </datafield>
  <datafield tag="245" ind1="1" ind2="0">
  <subfield code="a">Final British Offensive, August-November, 1918</subfield>
  </datafield>
  </record>',3.4174786475000007),
  (7,'<record xmlns="http://www.loc.gov/MARC21/slim">
  <datafield tag="034" ind1="0" ind2=" ">
  <subfield code="a">a</subfield>
  <subfield code="d">E002.318055</subfield>
  <subfield code="e">+004.484444</subfield>
  <subfield code="f">051.195833</subfield>
  <subfield code="g">N049.618333</subfield>
  <subfield code="9">A:dcx</subfield>
  </datafield>
  <datafield tag="245" ind1="1" ind2="0">
  <subfield code="a">Final British Offensive, August-November, 1918</subfield>
  </datafield>
  </record>',3.4174786475000007),
  (8,'<record xmlns="http://www.loc.gov/MARC21/slim">
  <datafield tag="034" ind1="0" ind2=" ">
  <subfield code="a">a</subfield>
  <subfield code="d">E00231.8055</subfield>
  <subfield code="e">+00448.4444</subfield>
  <subfield code="f">05119.5833</subfield>
  <subfield code="g">N04961.8333</subfield>
  <subfield code="9">A:dcx</subfield>
  </datafield>
  <datafield tag="245" ind1="1" ind2="0">
  <subfield code="a">Final British Offensive, August-November, 1918</subfield>
  </datafield>
  </record>',3.4174786475000007),
  (9,'<record xmlns="http://www.loc.gov/MARC21/slim">
  <datafield tag="034" ind1="0" ind2=" ">
  <subfield code="a">a</subfield>
  <subfield code="d">E0023180.55</subfield>
  <subfield code="e">+0044844.44</subfield>
  <subfield code="f">0511958.33</subfield>
  <subfield code="g">N0496183.33</subfield>
  <subfield code="9">A:dcx</subfield>
  </datafield>
  <datafield tag="245" ind1="1" ind2="0">
  <subfield code="a">Final British Offensive, August-November, 1918</subfield>
  </datafield>
  </record>',3.417478647500002),
 (10,NULL,0.0)
)
SELECT
  gid,
  ST_Area(ST_GeomFromMARC21(doc::text)) = area AS is_equal,
  ST_AsText(ST_GeomFromMARC21(doc::text)) AS geom
  
FROM j;


--WHERE ST_Area(box2d(ST_GeomFromMARC21(doc::text))) <> area;
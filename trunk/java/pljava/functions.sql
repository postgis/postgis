
-- Create a function to call the java function
CREATE OR REPLACE FUNCTION public.helloworld()
  RETURNS "varchar" AS
    'org.postgis.pljava.HelloWorld.helloWorld'
  LANGUAGE 'java' VOLATILE;  

SELECT sqlj.drop_type_mapping('public.geometry');

SELECT sqlj.add_type_mapping('geometry', 'org.postgis.pljava.PLJGeometry');

CREATE OR REPLACE FUNCTION public.getSize(geometry)
  RETURNS "int4" AS
    'org.postgis.pljava.HelloWorld.getSize'
  LANGUAGE 'java' IMMUTABLE STRICT;  

CREATE OR REPLACE FUNCTION public.getString(geometry)
  RETURNS "text" AS
    'org.postgis.pljava.HelloWorld.getString'
  LANGUAGE 'java' IMMUTABLE STRICT;  
  
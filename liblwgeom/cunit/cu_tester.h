#define MAX_CUNIT_ERROR_LENGTH 512
char cu_error_msg[MAX_CUNIT_ERROR_LENGTH+1];
void cu_error_msg_reset();

CU_pSuite register_measures_suite(void);
CU_pSuite register_geodetic_suite(void);
CU_pSuite register_libgeom_suite(void);
CU_pSuite register_cg_suite(void);
CU_pSuite register_homogenize_suite(void);
CU_pSuite register_out_gml_suite(void);
CU_pSuite register_out_kml_suite(void);
CU_pSuite register_out_geojson_suite(void);
CU_pSuite register_out_svg_suite(void);

int init_measures_suite(void);
int init_geodetic_suite(void);
int init_libgeom_suite(void);
int init_cg_suite(void);
int init_homogenize_suite(void);
int init_out_gml_suite(void);
int init_out_kml_suite(void);
int init_out_geojson_suite(void);
int init_out_svg_suite(void);

int clean_measures_suite(void);
int clean_geodetic_suite(void);
int clean_libgeom_suite(void);
int clean_cg_suite(void);
int clean_homogenize_suite(void);
int clean_out_gml_suite(void);
int clean_out_kml_suite(void);
int clean_out_geojson_suite(void);
int clean_out_svg_suite(void);


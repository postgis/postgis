#ifndef FLATCC_H
#define FLATCC_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * This is the primary `flatcc` interface when compiling `flatcc` as a
 * library. Functions and types in the this interface will be kept
 * stable to the extend possible or reasonable, but do not rely on other
 * interfaces except "config.h" used to set default options for this
 * interface.
 *
 * This interface is unrelated to the standalone flatbuilder library
 * which has a life of its own.
 */

#include <stddef.h>

#ifndef UINT8_MAX
#include <stdint.h>
#endif

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4820) /* x bytes padding added in struct */
#endif

typedef struct flatcc_options flatcc_options_t;
typedef void (*flatcc_error_fun) (void *err_ctx, const char *buf, size_t len);

struct flatcc_options {
    size_t max_schema_size;
    int max_include_depth;
    int max_include_count;
    int disable_includes;
    int allow_boolean_conversion;
    int allow_enum_key;
    int allow_enum_struct_field;
    int allow_multiple_key_fields;
    int allow_primary_key;
    int allow_scan_for_all_fields;
    int allow_string_key;
    int allow_struct_field_deprecate;
    int allow_struct_field_key;
    int allow_struct_root;
    int ascending_enum;
    int hide_later_enum;
    int hide_later_struct;
    int offset_size;
    int voffset_size;
    int utype_size;
    int bool_size;
    int require_root_type;
    int strict_enum_init;
    uint64_t vt_max_count;

    const char *default_schema_ext;
    const char *default_bin_schema_ext;
    const char *default_bin_ext;

    /* Code Generator specific options. */
    int gen_stdout;
    int gen_dep;

    const char *gen_depfile;
    const char *gen_deptarget;
    const char *gen_outfile;

    int gen_append;

    int cgen_pad;
    int cgen_sort;
    int cgen_pragmas;

    int cgen_common_reader;
    int cgen_common_builder;
    int cgen_reader;
    int cgen_builder;
    int cgen_verifier;
    int cgen_json_parser;
    int cgen_json_printer;
    int cgen_recursive;
    int cgen_spacing;
    int cgen_no_conflicts;


    int bgen_bfbs;
    int bgen_qualify_names;
    int bgen_length_prefix;

    /* Namespace args - these can override defaults so are null by default. */
    const char *ns;
    const char *nsc;

    const char **inpaths;
    const char **srcpaths;
    int inpath_count;
    int srcpath_count;
    const char *outpath;
};

/* Runtime configurable optoins. */
void flatcc_init_options(flatcc_options_t *opts);

typedef void *flatcc_context_t;

/*
 * Call functions below in order listed one at a time.
 * Each parse requires a new context.
 *
 * A reader file is named after the source base name, e.g.
 * `monster.fbs` becomes `monster.h`. Builders are optional and created
 * as `monster_builder.h`. A reader require a common header
 * `flatbuffers_commoner.h` and a builder requires
 * `flatbuffers_common_builder.h` in addition to the reader filers.  A
 * reader need no other source, but builders must link with the
 * `flatbuilder` library and include files in `include/flatbuffers`.
 *
 * All the files may also be concatenated into one single file and then
 * files will not be attempted included externally. This can be used
 * with stdout output. The common builder can follow the common
 * reader immediately, or at any later point before the first builder.
 * The common files should only be included once, but not harm is done
 * if duplication occurs.
 *
 * The outpath is prefixed every output filename. The containing
 * directory must exist, but the prefix may have text following
 * the directory, for example the namespace. If outpath = "stdout",
 * files are generated to stdout.
 *
 * Note that const char * options must remain valid for the lifetime
 * of the context since they are not copied. The options object itself
 * is not used after initialization and may be reused.
*/

/*
 * `name` is the name of the schema file or buffer. If it is path, the
 * basename is extracted (leading path stripped), and the default schema
 * extension is stripped if present. The resulting name is used
 * internally when generating output files. Typically the `name`
 * argument will be the same as a schema file path given to
 * `flatcc_parse_file`, but it does not have to be.
 *
 * `name` may be null if only common files are generated.
 *
 * `error_out` is an optional error handler. If null output is truncated
 * to a reasonable size and sent to stderr. `error_ctx` is provided as
 * first argument to `error_out` if `error_out` is non-zero, otherwise
 * it is ignored.
 *
 * Returns context or null on error.
 */
flatcc_context_t flatcc_create_context(flatcc_options_t *options, const char *name,
        flatcc_error_fun error_out, void *error_ctx);

/* Like `flatcc_create_context`, but with length argument for name. */
/*
 * Parse is optional - not needed for common files. If the input buffer version
 * is called, the buffer must be zero terminated, otherwise an input
 * path can be specified. The output path can be null.
 *
 * Only one parse can be called per context.
 *
 * The buffer size is limited to the max_schema_size option unless it is
 * 0. The default is reasonable size like 64K depending on config flags.
 *
 * The buffer must remain valid for the duration of the context.
 *
 * The schema cannot contain include statements when parsed as a buffer.
 *
 * Returns 0 on success.
 */
int flatcc_parse_buffer(flatcc_context_t ctx, const char *buf, size_t buflen);

/*
 * If options contain a non-zero `inpath` option, the resulting filename is
 * prefixed with that path unless the filename is an absolute path.
 *
 * Errors are sent to the error handler given during initialization,
 * or to stderr.
 *
 * The file size is limited to the max_schema_size option unless it is
 * 0. The default is reasonable size like 64K depending on config flags.
 *
 * Returns 0 on success.
 */
int flatcc_parse_file(flatcc_context_t ctx, const char *filename);

/*
 * Generate output files. The basename derived when the context was
 * created is used used to name the output files with respective
 * extensions. If the outpath option is not null it is prefixed the
 * output files. The `cgen_common_reader, cgen_common_builder,
 * cgen_reader, and cgen_builder` must be set or reset depending on what
 * is to be generated. The common files do not require a parse, and the
 * non-common files require a successfull parse or the result is
 * undefined.
 *
 * Unlinke the parser, the code generator produce errors to stderr
 * always. These errors are rare, such as using too long namespace
 * names.
 *
 * If the `gen_stdout` option is set, all files are generated to stdout.
 * In this case it is unwise to mix C and binary schema output options.
 *
 * If `bgen_bfbs` is set, a binary schema is generated to a file with
 * the `.bfbs` extension. See also `flatcc_generate_binary_schema` for
 * further details. Only `flatcc_generate_files` is called via the
 * `flatcc` cli command.
 *
 * The option `bgen_length_prefix` option will cause a length prefix to be
 * written to the each output binary schema. This option is only
 * understood when writing to files.
 *
 * Returns 0 on success.
 */
int flatcc_generate_files(flatcc_context_t ctx);

/*
 * Returns a buffer with a binary schema for a previous parse.
 * The user is responsible for calling `free` on the returned buffer
 * unless it returns 0 on error.
 *
 * Can be called instead of generate files, before, or after, but a
 * schema must be parsed first.
 *
 * Returns a binary schema in `reflection.fbs` format. Any included
 * files will be contained in the schema and there are no separate
 * schema files for included schema.
 *
 * All type names are scoped, mening that they are refixed their
 * namespace using `.` as the namespace separator, for example:
 * "MyGame.Example.Monster". Note that the this differs from the current
 * `flatc` compiler which does not prefix names. Enum names are not
 * scoped, but the scope is implied by the containing enum type.
 * The option `bgen_qualify_names=0` changes this behavior.
 *
 * If the default option `ascending_enum` is disabled, the `flatcc` will
 * accept duplicate values and overlapping ranges like the C programming
 * language. In this case enum values in the binary schema will not be
 * searchable. At any rate enum names are not searchable in the current
 * schema format.
 *
 */
void *flatcc_generate_binary_schema(flatcc_context_t ctx, size_t *size);

/*
 * Similar to `flatcc_generate_binary_schema` but copies the binary
 * schema into a user supplied buffer. If the buffer is too small
 * the return value will be negative and the buffer content undefined.
 */
int flatcc_generate_binary_schema_to_buffer(flatcc_context_t ctx, void *buf, size_t bufsiz);

/* Must be called to deallocate resources eventually - it valid but
 * without effect to call with a null context. */
void flatcc_destroy_context(flatcc_context_t ctx);

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#ifdef __cplusplus
}
#endif

#endif /* FLATCC_H */

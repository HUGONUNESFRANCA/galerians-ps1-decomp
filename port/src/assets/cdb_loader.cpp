/*
 * Assets — CDB Loader
 *
 * Will load the pre-extracted CDB sub-files produced by
 * tools/cdb_extractor.py (see docs/asset_format.md → "CDB Format").
 *
 * On PS1 the runtime decompresses LZSS at load time; the PC port
 * ships the already-decompressed assets so this module is just
 * filesystem I/O + sub-file lookup by index. Empty translation unit
 * until the asset path layout is finalized.
 */

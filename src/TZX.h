void TZX_Output( struct Spectrum_CassetteBlock *list, char *filename );
unsigned char *TZX_Parse( unsigned char *data, int size );
void TZX_AddPauseBlock( struct Spectrum_CassetteBlock *node, FILE *file );


// This is a dirty hack to get around a problem in the m68k build...// I believe somewhere <ctype.h> needs to be included in a few SDL src files.// But, for now, this will do.int tolower(char X){  return (((X) >= 'A') && ((X) <= 'Z') ? ('a'+((X)-'A')) : (X));}
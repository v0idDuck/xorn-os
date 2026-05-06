#include <xeapi.h>
// xpil apps\calc.c to-disk\bin\calc.xe
// in xorn: run bin/calc.xe

void entry(unsigned long long dummy, XornAPI* api) {
    xorn_init(api);
    XeSetting xes;
    xes.Fontsize = 3;
    xes.cx = 40;
    xes.cy = 40;
    api->clear_screen(0x000000);
    
    RDATA title[] = "SIMPLE XORN CALC";
    RDATA fmsg[] = "ENTER FIRST NUMBER(0-9): ";
    RDATA smsg[] = "ENTER SECOND NUMBER(0-9): ";
    RDATA emsg[] = "INVALID INPUT!";
    RDATA rmsg[] = "RESULT: ";
    xe_printsl(xes, title, 0xFFFFFF);
    xe_prints(xes, fmsg, 0xFFFFFF);
    char fs = 0, ss = 0;
    fs = xread_key();
    char fshow[2]; fshow[0] = fs; fshow[1] = 0;
    xe_printsl(xes, fshow, 0xFFFFFF);

    xe_prints(xes, smsg, 0xFFFFFF);
    ss = xread_key();
    char sshow[2]; sshow[0] = ss; sshow[1] = 0;
    xe_printsl(xes, sshow, 0xFFFFFF);
    int fnum = fs - '0';
    int snum = ss - '0';
    int result = fnum + snum;
    xe_prints(xes, rmsg, 0xFFFFFF);
    char ress[16];
    _int_to_str(result, ress);
    xe_prints(xes, ress, 0xFFFFFF); 
}
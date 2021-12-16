/**
 * modio - modbus input output command line tool
 *
 *  A generic tool for read and write modbus registers. It supports modbus
 *  RTU and modbus TCP connections. Device and register information can be
 *  supplied as external structured configuration files and used to format
 *  the read data.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <math.h>
#include <modbus.h>
#include <libconfig.h>
#include <hashmap.h>
#include "modio.h"

/* register store arrays */
uint16_t ireg[REG_SIZE];    /* store input registers*/
uint16_t hreg[REG_SIZE];    /* store holding registers*/
uint8_t creg[REG_SIZE];     /* store coil registers */
uint8_t ibreg[REG_SIZE];    /* store input bit registers */

/* return a string with binary representation of inum */
char *int_to_bin(uint inum);

/* convert an integer to string */
char *int_to_str(int inum);

/* convert a hex to string */
char *hex_to_str(int xnum);

/* convert an array of words into a string of '.' separated bytes */
char *mem_to_bytes(uint16_t *array, int size, char *(*conv)(int));

/* initialize read register array */
void init_rrega();

/* convert an array of words with ASCII bytes into a string*/
char *words_to_str(const uint16_t *array, int length);

/* Concatenate and invert 16bit words to 32bit (length = 2) or 64bit (length = 4) */
uint64_t concat_inv16(const uint16_t *array, int length);

/* create a new modbus context */
modbus_t *modbus_new(char *port, serconf_t sc);

/* initialize modbus connection */
modbus_t *modbus_init(char *port, serconf_t sc, int id);

/* initialize supported devices' register list */
int init_drlist(dvlist_t **lst);

/* read supported devices and registers' info */
void read_dreg(dvlist_t *lst);

/* print supported devices' info */
void print_dev_info(dvlist_t *lst, int sz);

/* print supported device registers' info */
void print_dev_reginfo(dvlist_t *lst, int num, int nor);

/* read device registers */
void read_dev_regs(modbus_t *mb, dvlist_t *dvl, int dnum);

/* print the program usage */
void usage(char *pname);

/*
 * main
 */
int main(int argc, char **argv)
{
    int rval = 0;               /* return value */
    int lsz = 0;                /* device list size */
    dvlist_t *dvl;              /* the supported devices' list */
    modbus_t *mb;               /* modbus context */
    HASHMAP(char, struct dreg) regmap;

    /*
     * option context variables
     */
    int nb = 0;                 /* number of registers */
    int addr = 0x0;             /* register address */
    int *addr_l = NULL;         /* list of addresses */
    int addr_c = 0;             /* count of addresses */
    int *rgnum = NULL;          /* list of register numbers */
    char *port = NULL;          /* port to connect */

    char *tkn;                  /* temp token pointer for strtok */
    int rread = FALSE;          /* register read flag */
    int rwrite = FALSE;         /* register write flag */
    int len = 1;                /* len of read or write */
    int mget = FALSE;           /* get memory region flag */
    int rgac = FALSE;           /* register access */
    int bin = FALSE;            /* print values as bin */
    int hex = FALSE;            /* print values as hex */
    int str = FALSE;            /* print values as string */
    int id = MODBUS_SLAVE_ID;   /* modbus slave id */
    int zba = FALSE;            /* zero based addressing flag */
    int jot = FALSE;            /* jotting device's info flag */
    int dnum = 0;               /* device number for printing registers' info */
    int hlo = 0;                /* print value as high low word */
    int bfm = 0;                /* print value formatted as '.' separated bytes */
    prfmt_t pfm = DEC;          /* print values as dec */
    regtype_t rtype = COIL;     /* holds register type for read */
    serconf_t sc = {
            BAUD_RATE,
            PARITY,
            STOP_BIT,
            DATA_BIT
    };                          /* serial configuration */
    uint16_t val = 0;           /* value to write */

    /* initialize the device list */
    lsz = init_drlist(&dvl);

    /* read device and registers' info from device files */
    read_dreg(dvl);

    /* initialize register area */
    init_rrega();

    /* if no option specified print usage */
    if (argc == 1) {
        usage(argv[0]);
        exit(EXIT_SUCCESS);
    }

    /* check options */
    int opt;
    while ((opt = getopt(argc, argv, "p:u:y:o:d:i:za:rw:l:gt:bxsf:nv:ce:j:h")) != -1) {
        switch (opt) {
            case 'p':
                port = malloc(sizeof(optarg));
                strcpy(port, optarg);
                break;
            case 'u':
                sc.baud = (int )strtoul(optarg, NULL, 10);
                break;
            case 'y':
                sc.prty = *(char *)optarg;
                break;
            case 'o':
                sc.sbit = (int )strtoul(optarg, NULL, 10);
                break;
            case 'd':
                sc.dbit = (int )strtoul(optarg, NULL, 10);
                break;
            case 'i':
                id = (int )strtoul(optarg, NULL, 0);
                break;
            case 'z':
                zba = TRUE;
                break;

            /* get comma separated regs/addresses in addr_l array */
            case 'a':
                tkn = optarg;
                while (*tkn) {              /* calculate number of ',' */
                    if (',' == *tkn) {
                        addr_c++;           /* increase reg/address counter */
                    }
                    tkn++;
                }

                /* allocate memory for addr_l array with size addr_c */
                addr_l = (int *)malloc(sizeof(int) * (addr_c + 1));
                tkn = strtok(optarg, ",");
                *addr_l = (int )strtoul(tkn, NULL, 0);
                if (addr_c != 0) {
                    int i = 1;
                    while ((tkn = strtok(NULL, ",")) != NULL) {
                        *(addr_l + i) = (int )strtoul(tkn, NULL, 0);
                        i++;
                    }
                }
                break;
            case 'r':
                rread = TRUE;
                break;
            case 'w':
                rwrite = TRUE;
                val = strtol(optarg, NULL, 10);
                break ;
            case 'l':
                len = (int )strtoul(optarg, NULL, 10);
                break;
            case 't':
                rtype = strtol(optarg, NULL, 10);
                break;

            /* use registers instead of addresses */
            case 'g':
                rgac = TRUE;
                break;

            /* print registers in binary format */
            case 'b':
                bin = TRUE;
                pfm = BIN;
                break;

            /* print registers in hex format */
            case 'x':
                hex = TRUE;
                pfm = HEX;
                break;

            /* print registers as ascii encoded */
            case 's':
                str = TRUE;
                pfm = ASC;
                break;

            /* print registers as dot separated bytes 1:dec, 2:hex */
            case 'f':
                bfm = (int )strtoul(optarg, NULL, 0);
                if (bfm < 1 || bfm  > 2) {
                    usage(argv[0]);
                    exit(EXIT_FAILURE);
                }
                if (bfm == 1) {
                    pfm = BFD;
                } else {
                    pfm = BFX;
                }
                break;

            /* print registers as hi low words (length must be even) */
            case 'n':
                hlo = TRUE;
                pfm = HLO;
                break;

            /* print register info from support register file */
            case 'v':
                dnum = (int )strtoul(optarg, NULL, 0);
                break;

            /* read all registers defined in support register file */
            case 'j':
                jot = TRUE;
                dnum = (int )strtoul(optarg, NULL, 0);

                /* if device number greater than device list size exit */
                if (dnum > lsz || dnum == 0) {
                    usage(argv[0]);
                    exit(EXIT_FAILURE);
                }
                break;

            /* print list of supported devices */
            case 'c':
                print_dev_info(dvl, lsz);
                exit(EXIT_SUCCESS);

            /* print registers of a supported device */
            case 'e':
                dnum = (int )strtoul(optarg, NULL, 0);

                /* if device number greater than device list size exit */
                if (dnum > lsz || dnum == 0) {
                    usage(argv[0]);
                    exit(EXIT_FAILURE);
                }
                print_dev_reginfo(dvl, dnum - 1, dvl[dnum-1].nor);
                exit(EXIT_SUCCESS);
            case 'h':
                usage(argv[0]);
                exit(EXIT_SUCCESS);
            default:
                usage(argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    /* check if there are non option arguments */
    for (int i = optind; i < argc; i++) {
        printf("non option argument %s\n", argv[i]);
        usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    /* use DEVICE_PATH if port hasn't been defined */
    if (port == NULL) {
        port = (char *)malloc(sizeof(DEVICE_PATH));
        strcpy(port, DEVICE_PATH);
    }

    /* if device number greater than device list size exit */
    if (dnum > lsz) {
        usage(argv[0]);
        exit(EXIT_SUCCESS);
    }

    /* if -j <dev_num> read device registers defined in configuration file */
    if (jot && dnum) {

        /* initialize modbus connection */
        mb = modbus_init(port, sc, id);
        if (mb == NULL) {
            exit(EXIT_FAILURE);
        }
        read_dev_regs(mb, dvl, dnum - 1);
        exit(EXIT_SUCCESS);
    }

    /* if -v <num>, create a hashmap between register number and dreg struct for device <num> in device list */
    if (dnum) {

        /* Initialize with default string key hash function and comparator */
        hashmap_init(&regmap, hashmap_hash_string, strcmp);
        dreg_t *regs = dvl[dnum - 1].regs;

        /* Load reg data into the regmap */
        int r;
        for (int i = 0; i < dvl[dnum - 1].nor; i++) {
            char *key = strcat(int_to_str(regs[i].type), int_to_str(regs[i].num));
            r = hashmap_put(&regmap, key, &regs[i]);
            if (r < 0) {

                /* Expect -EEXIST return value for duplicates */
                printf("putting reg[%d] failed: %s\n", regs[i].num, strerror(-r));
            }
        }
    }

   /* allocate memory for register array */
    rgnum = (int *)malloc(sizeof(int) * (addr_c + 1));

    /* if -g, register based access is enabled calculate register access address */
    if (rgac) {

        /* loop over the register list */
        for (int i = 0; i <= addr_c; i++) {

            /* calculate the digits of the register number and allocate the proper memory size */
            char *hexreg = NULL;
            if (addr_l[i] != 0) {
                hexreg = (char *)malloc(sizeof(char) * (int) (log10(addr_l[i]) + 1));
            } else {
                hexreg = (char *)malloc(sizeof(char));
            }
            sprintf(hexreg, "%x", addr_l[i]);
            rgnum[i] = (int )(0x0 + strtoul(hexreg, NULL, 16));
            switch (rtype) {
                case COIL:
                    addr_l[i] = 0x0 + rgnum[i];
                    break;
                case INPUT_B:
                    addr_l[i] = 0x10000 + rgnum[i];
                    break;
                case INPUT_R:
                    addr_l[i] = 0x30000 + rgnum[i];
                    break;
                case HOLDING:
                    addr_l[i] = 0x40000 + rgnum[i];
                    break;
                default:
                    usage(argv[0]);
                    exit(EXIT_FAILURE);
            }
        }

    /* otherwise calculate register number from address and store is to rgnum array */
    } else {
        for (int i = 0; i <= addr_c; i++) {

            /* if address type access calculate rtype */
            char *xaddr = hex_to_str(addr_l[i]);
            rtype = (xaddr[0] - '0') - 1;
            switch (rtype) {
                case COIL:
                    rgnum[i] = addr_l[i] - 0x0;
                    break;
                case INPUT_B:
                    rgnum[i] = addr_l[i] - 0x10000;
                    break;
                case INPUT_R:
                    rgnum[i] = addr_l[i] - 0x30000;
                    break;
                case HOLDING:
                    rgnum[i] = addr_l[i] - 0x40000;
                    break;
                default:
                    usage(argv[0]);
                    exit(EXIT_FAILURE);
            }
        }
    }

    /* calculate register address using zero based addressing */
    for (int i = 0; i <= addr_c; i++) {
        addr_l[i] = addr_l[i] - zba;
    }

    /* make current address first address in addr_l array of regs/addresses */
    addr = addr_l[0];

    /* initialize modbus connection */
    mb = modbus_init(port, sc, id);
    if (mb == NULL) {
        exit(EXIT_FAILURE);
    }

    /* if -w <data> and -t 0|3 write <data> to <address> */
    if (rwrite == TRUE && rtype == HOLDING) {
        rval = modbus_write_register(mb, addr, val);
        if (rval == -1) {
            printf("ERROR:(%s) modbus_write_register addr:0x%08x, path:%s\n",
                   modbus_strerror(errno),
                   addr,
                   port
            );
            exit(EXIT_FAILURE);
        }
    } else if (rwrite == TRUE && rtype == COIL) {
        rval = modbus_write_bit(mb, addr, val);
        if (rval == -1) {
            printf("ERROR:(%s) modbus_write_bit addr:0x%08x, path:%s\n",
                   modbus_strerror(errno),
                   addr,
                   port
            );
            exit(EXIT_FAILURE);
        }
    } else if (rwrite == TRUE) {
        printf("Invalid type of register to write\n");
        usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    /* if -r (read register/address) */
    if (rread == TRUE) {
        uint16_t *reg16p;   /* pointer to 16bit register */
        uint8_t *reg8p;     /* pointer to 8bit register */

        /* loop over all addresses or registers */
        for (int i = 0; i <= addr_c; i++) {
            addr = addr_l[i];
            if (rgac == FALSE) {
                char *xaddr = hex_to_str(addr);
                rtype = (xaddr[0] - '0') - 1;
            }

            /* set print format to decimal */
            int prfmt = 2;
            if (dnum != 0) {
                len = (hashmap_get(&regmap, strcat(int_to_str(rtype), int_to_str(rgnum[i]))) ?
                       hashmap_get(&regmap, strcat(int_to_str(rtype), int_to_str(rgnum[i])))->len :
                       len
                       );
                pfm = (hashmap_get(&regmap, strcat(int_to_str(rtype), int_to_str(rgnum[i]))) ?
                       hashmap_get(&regmap, strcat(int_to_str(rtype), int_to_str(rgnum[i])))->prfmt :
                       prfmt
                );
            }
            switch (rtype) {
                case COIL:
                case INPUT_B:
                    if (rtype == COIL) {
                            rval = modbus_read_bits(mb, addr, len, creg);
                            reg8p = creg;
                    } else {
                        rval = modbus_read_input_bits(mb, addr, len, ibreg);
                        reg8p = ibreg;
                    }
                    if (rval == -1) {
                        printf("ERROR:(%s) modbus_read_xx addr:0x%x, count: %d, path: %s\n",
                               modbus_strerror(errno),
                               addr,
                               len,
                               port
                               );
                        exit(EXIT_FAILURE);
                    } else {
                        for (int j = 0; j < len; j++) {
                            if (pfm == BIN) {
                                if (dnum) {
                                    printf("reg: %03d name: %-35s address: 0x%08x value: %16s\n",
                                           rgnum[i] + j,
                                           ((hashmap_get(&regmap, strcat(int_to_str(rtype),
                                                                         int_to_str(rgnum[i])))) ?
                                             hashmap_get(&regmap, strcat(int_to_str(rtype),
                                                                         int_to_str(rgnum[i])))->name :
                                             "UNDEFINED"),
                                           addr,
                                           int_to_bin(*(uint16_t *) reg8p)
                                    );
                                } else {
                                    printf("reg: %03d address: 0x%08x value: %16s\n",
                                           rgnum[i] + j,
                                           addr,
                                           int_to_bin(*(uint8_t *) reg8p)
                                    );
                                }
                            } else if (pfm == HEX) {
                                if (dnum) {
                                    printf("reg: %03d name: %-35s address: 0x%08x value: 0x%x\n",
                                           rgnum[i] + j,
                                           ((hashmap_get(&regmap, strcat(int_to_str(rtype),
                                                                         int_to_str(rgnum[i])))) ?
                                            hashmap_get(&regmap, strcat(int_to_str(rtype),
                                                                        int_to_str(rgnum[i])))->name :
                                            "UNDEFINED"),
                                           addr,
                                           *(uint8_t *) reg8p
                                    );
                                } else {
                                    printf("reg: %03d address: 0x%08x value: 0x%x\n",
                                           rgnum[i] + j,
                                           addr,
                                           *(uint8_t *) reg8p
                                    );
                                }
                            } else if (pfm == ASC) {
                                if (dnum) {
                                    printf("reg: %03d name: %-35s address: 0x%08x value: %s\n",
                                           rgnum[i] + j,
                                           ((hashmap_get(&regmap, strcat(int_to_str(rtype),
                                                                         int_to_str(rgnum[i] + j)))) ?
                                             hashmap_get(&regmap, strcat(int_to_str(rtype),
                                                                        int_to_str(rgnum[i] + j)))->name :
                                             "UNDEFINED"),
                                           addr,
                                           (char *) reg8p
                                    );
                                } else {
                                    printf("reg: %03d address: 0x%08x value: %s\n",
                                           rgnum[i] + j,
                                           addr,
                                           (char *) reg8p
                                    );
                                }
                                break;
                            } else if (pfm == DEC) {
                                if (dnum) {
                                    printf("reg: %03d name: %-35s address: 0x%08x value: %.2f%s\n",
                                           rgnum[i] + j,
                                           ((hashmap_get(&regmap, strcat(int_to_str(rtype),
                                                                         int_to_str(rgnum[i])))) ?
                                             hashmap_get(&regmap, strcat(int_to_str(rtype),
                                                                        int_to_str(rgnum[i])))->name :
                                             "UNDEFINED"),
                                           addr,
                                           *(uint16_t *)reg8p *
                                           ((hashmap_get(&regmap, strcat(int_to_str(rtype),
                                                                         int_to_str(rgnum[i])))) ?
                                             hashmap_get(&regmap,strcat(int_to_str(rtype),
                                                                        int_to_str(rgnum[i])))->scale : 1),
                                           ((hashmap_get(&regmap, strcat(int_to_str(rtype),
                                                                         int_to_str(rgnum[i])))) ?
                                             hashmap_get(&regmap, strcat(int_to_str(rtype),
                                                                         int_to_str(rgnum[i])))->engu :
                                             "")
                                    );
                                } else {
                                    printf("reg: %03d address: 0x%08x value: %d\n",
                                           rgnum[i] + j,
                                           addr, *(uint8_t *) reg8p
                                    );
                                }
                            }
                            reg8p++;
                            addr++;
                        }
                    }
                    break;
                case INPUT_R:
                case HOLDING:
                    if (rtype == HOLDING) {
                        rval = modbus_read_registers(mb, addr, len, hreg);
                        reg16p = hreg;
                    } else {
                        rval = modbus_read_input_registers(mb, addr, len, ireg);
                        reg16p = ireg;
                    }
                    if (rval == -1) {
                        printf("ERROR:(%s) modbus_read_xx addr: 0x%x count:%d path:%s\n",
                               modbus_strerror(errno),
                               addr,
                               len,
                               port
                        );
                        exit(EXIT_FAILURE);
                    } else {
                        for (int j = 0; j < len; j++) {
                            if (pfm == BIN) {
                                if (dnum) {
                                    printf("reg: %03d name: %-35s address: 0x%08x value: %16s\n",
                                           rgnum[i] + j,
                                           ((hashmap_get(&regmap, strcat(int_to_str(rtype),
                                                                         int_to_str(rgnum[i])))) ?
                                             hashmap_get(&regmap, strcat(int_to_str(rtype),
                                                                        int_to_str(rgnum[i])))->name :
                                             "UNDEFINED"),
                                           addr,
                                           int_to_bin(*(uint16_t *) reg16p)
                                    );
                                } else {
                                    printf("reg: %03d address: 0x%08x value: %16s\n",
                                           rgnum[i] + j,
                                           addr,
                                           int_to_bin(*(uint16_t *) reg16p)
                                    );
                                }
                            } else if (pfm == HEX) {
                                if (dnum) {
                                    printf("reg: %03d name: %-35s address: 0x%08x value: 0x%x\n",
                                           rgnum[i] + j,
                                           ((hashmap_get(&regmap, strcat(int_to_str(rtype),
                                                                         int_to_str(rgnum[i])))) ?
                                             hashmap_get(&regmap, strcat(int_to_str(rtype),
                                                                        int_to_str(rgnum[i])))->name :
                                             "UNDEFINED"),
                                           addr,
                                           *(uint16_t *) reg16p
                                    );
                                } else {
                                    printf("reg: %03d address: 0x%08x value: 0x%x\n",
                                           rgnum[i] + j,
                                           addr,
                                           *(uint16_t *) reg16p
                                    );
                                }
                            } else if (pfm == HEX) {
                                char *s = words_to_str(reg16p, len);
                                if (dnum) {
                                    printf("reg: %03d name: %-35s address: 0x%08x value: %s\n",
                                           rgnum[i] + j,
                                           ((hashmap_get(&regmap, strcat(int_to_str(rtype),
                                                                         int_to_str(rgnum[i])))) ?
                                             hashmap_get(&regmap, strcat(int_to_str(rtype),
                                                                        int_to_str(rgnum[i])))->name :
                                             "UNDEFINED"),
                                           addr,
                                           s
                                    );
                                } else {
                                    printf("reg: %03d address: 0x%08x value: %s\n",
                                           rgnum[i] + j,
                                           addr,
                                           s
                                    );
                                }
                                break;
                            } else if (bfm == BFD || bfm == BFX) {
                                char *s = (bfm == BFD) ? mem_to_bytes(reg16p, len, int_to_str)
                                                       : mem_to_bytes(reg16p, len, hex_to_str);
                                if (dnum) {
                                    printf("reg: %03d name: %-35s address: 0x%08x value: %s\n",
                                           rgnum[i] + j,
                                           ((hashmap_get(&regmap, strcat(int_to_str(rtype),
                                                                         int_to_str(rgnum[i])))) ?
                                            hashmap_get(&regmap, strcat(int_to_str(rtype),
                                                                        int_to_str(rgnum[i])))->name :
                                            "UNDEFINED"),
                                           addr,
                                           s
                                    );
                                } else {
                                    printf("reg: %03d address: 0x%08x value: %s\n",
                                           rgnum[i] + j,
                                           addr,
                                           s
                                    );
                                }
                                break;
                            } else if (pfm == HLO) {
                                if (len % 2 != 0) {
                                    printf("Error, not aligned memory size\n");
                                    break;
                                }
                                int hlw = len / 2;
                                for (int k = 0; k < hlw; k++) {
                                    if (dnum) {
                                        printf("reg: %03d name: %-35s address: 0x%08x value: %li%s\n",
                                               rgnum[i] + 2 * k,
                                               ((hashmap_get(&regmap, strcat(int_to_str(rtype),
                                                                             int_to_str(rgnum[i])))) ?
                                                 hashmap_get(&regmap, strcat(int_to_str(rtype),
                                                                            int_to_str(rgnum[i])))->name :
                                                 "UNDEFINED"),
                                               addr + 2 * k,
                                               concat_inv16(reg16p, 2),
                                               ((hashmap_get(&regmap, strcat(int_to_str(rtype),
                                                                             int_to_str(rgnum[i])))) ?
                                                 hashmap_get(&regmap, strcat(int_to_str(rtype),
                                                                            int_to_str(rgnum[i])))->engu :
                                                 "")
                                        );
                                    } else {
                                        printf("reg: %03d address: 0x%08x value: %li\n",
                                               rgnum[i] + 2 * k,
                                               addr,
                                               concat_inv16(reg16p, 2)
                                        );
                                    }
                                    reg16p += 2;
                                }
                                break;
                            } else if (pfm == DEC) {
                                if (dnum) {
                                    printf("reg: %03d name: %-35s address: 0x%08x value: %.2f%s\n",
                                           rgnum[i] + j,
                                           ((hashmap_get(&regmap, strcat(int_to_str(rtype),
                                                                         int_to_str(rgnum[i])))) ?
                                            hashmap_get(&regmap, strcat(int_to_str(rtype),
                                                                        int_to_str(rgnum[i])))->name :
                                            "UNDEFINED"),
                                           addr,
                                           *(uint16_t *) reg16p *
                                           ((hashmap_get(&regmap, strcat(int_to_str(rtype),
                                                                         int_to_str(rgnum[i])))) ?
                                            hashmap_get(&regmap, strcat(int_to_str(rtype),
                                                                        int_to_str(rgnum[i])))->scale : 1),
                                           ((hashmap_get(&regmap, strcat(int_to_str(rtype),
                                                                         int_to_str(rgnum[i])))) ?
                                            hashmap_get(&regmap, strcat(int_to_str(rtype),
                                                                        int_to_str(rgnum[i])))->engu :
                                            "")
                                    );
                                } else {
                                    printf("reg: %03d address: 0x%08x value: %d\n",
                                           rgnum[i] + j,
                                           addr,
                                           *(uint16_t *) reg16p
                                    );
                                }
                            }
                            reg16p++;
                            addr++;
                        }
                    }
                    break;
                default:
                    usage( argv[0]);
                    exit(EXIT_FAILURE);
            }
        }
        exit(EXIT_SUCCESS);
    }
    modbus_close(mb);
    modbus_free(mb);
    exit(EXIT_SUCCESS);
}

/*
 * Read device registers
 */
void read_dev_regs(modbus_t *mb, dvlist_t *dvl, int dnum)
{
    uint16_t *reg16p;   /* pointer to 16bit register */
    uint8_t *reg8p;     /* pointer to 8bit register */
    int addr;
    int rval;

    printf("%s %s %s:\n", dvl[dnum].type, dvl[dnum].manfc, dvl[dnum].model);
    printf("%-4s %-35s %-10s %-8s\n", "REG", "NAME", "ADDRESS", "VALUE");
    dreg_t *r = dvl[dnum].regs;
    for (int i = 0; i < dvl[dnum].nor; i++) {
        switch(r[i].type) {
            case COIL:
            case INPUT_B:
                if (r[i].type == COIL) {
                    addr = 0x0 + r[i].num - dvl[dnum].zba;
                    rval = modbus_read_bits(mb, addr, r[i].len, creg);
                    reg8p = creg;
                } else {
                    addr = 0x10000 + r[i].num - dvl[dnum].zba;
                    rval = modbus_read_input_bits(mb, addr, r[i].len, ibreg);
                    reg8p = ibreg;
                }
                if (rval == -1) {
                    printf("ERROR:(%s) modbus_read_xx addr:0x%x, count: %d\n",
                           modbus_strerror(errno),
                           addr,
                           r[i].len
                    );
                    exit(EXIT_FAILURE);
                } else {
                    for (int j = 0; j < r[i].len; j++) {
                        if (r[i].prfmt == BIN) {
                            printf("%04d %-35s 0x%08x %s\n",
                                   r[i].num + j,
                                   r[i].name,
                                   r[i].addr + j,
                                   int_to_bin(*(uint16_t *) reg8p)
                            );
                        } else if (r[i].prfmt == HEX) {
                            printf("%04d %-35s 0x%08x 0x%x\n",
                                   r[i].num + j,
                                   r[i].name,
                                   r[i].addr + j,
                                   *reg8p
                            );
                        } else if (r[i].prfmt == ASC) {
                            printf("%04d %-35s 0x%08x %s\n",
                                   r[i].num + j,
                                   r[i].name,
                                   r[i].addr + j,
                                   (char *) reg8p
                            );
                        } else {
                            printf("%04d %-35s 0x%08x %d\n",
                                   r[i].num + j,
                                   r[i].name,
                                   r[i].addr + j,
                                   *reg8p
                            );
                        }
                        reg8p++;
                    }
                }
                break;
            case INPUT_R:
            case HOLDING:
                if (r[i].type == HOLDING) {
                    addr = 0x40000 + r[i].num - dvl[dnum].zba;
                    rval = modbus_read_registers(mb, addr, r[i].len, hreg);
                    reg16p = hreg;
                } else {
                    addr = 0x30000 + r[i].num - dvl[dnum].zba;
                    rval = modbus_read_input_registers(mb, addr, r[i].len, ireg);
                    reg16p = ireg;
                }
                if (rval == -1) {
                    printf("ERROR:(%s) modbus_read_xx addr:0x%x, count: %d\n",
                           modbus_strerror(errno),
                           addr,
                           r[i].len
                    );
                    exit(EXIT_FAILURE);
                }
                for (int j = 0; j < r[i].len; j++) {
                    if (r[i].prfmt == BIN) {
                        printf("%04d %-35s 0x%08x %s\n",
                               r[i].num,
                               r[i].name,
                               r[i].addr + j,
                               int_to_bin(*(uint16_t *) reg16p)
                        );
                    } else if (r[i].prfmt == HEX) {
                        printf("%04d %-35s 0x%08x 0x%x\n",
                               r[i].num,
                               r[i].name,
                               r[i].addr + j,
                               *reg16p
                        );
                    } else if (r[i].prfmt == ASC) {
                        char *s = words_to_str(reg16p, r[i].len);
                        printf("%04d %-35s 0x%08x %s\n",
                               r[i].num,
                               r[i].name,
                               r[i].addr + j,
                               s
                        );
                        break;
                    } else if (r[i].prfmt == BFX) {
                        char *s = mem_to_bytes(reg16p, r[i].len, hex_to_str);
                        printf("%04d %-35s 0x%08x %s\n",
                               r[i].num,
                               r[i].name,
                               r[i].addr + j,
                               s
                        );
                        break;
                    } else if (r[i].prfmt == BFD) {
                        char *s = mem_to_bytes(reg16p, r[i].len, int_to_str);
                        printf("%04d %-35s 0x%08x %s\n",
                               r[i].num,
                               r[i].name,
                               r[i].addr + j,
                               s
                        );
                        break;
                    } else if (r[i].prfmt == HLO) {
                        if (r[i].len%2 != 0) {
                            printf("Error, not aligned memory size\n");
                            break;
                        }
                        int hlw = r[i].len / 2;
                        for (int k = 0; k < hlw; k++) {
                            printf("%04d %-35s 0x%08x %.2f%s\n",
                                   r[i].num + 2 * k,
                                   r[i].name,
                                   r[i].addr + 2 * k,
                                   concat_inv16(reg16p, 2) * r[i].scale,
                                   r[i].engu
                            );
                            reg16p += 2;
                        }
                       break;
                    } else {
                        if (r[i].len == 2) {
                            printf("%04d %-35s 0x%08x %li%s\n",
                                   r[i].num,
                                   r[i].name,
                                   r[i].addr + j,
                                   concat_inv16(reg16p, r[i].len),
                                   r[i].engu
                            );
                            break;
                        } else {
                            printf("%04d %-35s 0x%08x %.2f%s\n",
                                   r[i].num + j,
                                   r[i].name,
                                   r[i].addr + j,
                                   *reg16p * r[i].scale,
                                   r[i].engu
                            );
                        }
                    }
                    reg16p++;
                }
                break;
            default:
                exit(EXIT_FAILURE);
        }
    }
}


/*
 * format a memory of words into a string of '.' separated bytes.
 * bytes in words are swapped and converted by char *(*conv)(int) func
 * accordingly
 *
 * word 0: byte01.byte00
 * word 1: byte11.byte10
 *
 * string: byte00.byte01.byte10.byte11
 *
 *
 */
char *mem_to_bytes(uint16_t *array, int size, char *(*conv)(int)) {
    int slen = 0;
    unsigned char *p = (unsigned char *)array;

    for (int i = 0; i < size * 2; i++) {
        slen += snprintf(NULL, 0, "%i", p[i]);
    }
    char *s = (char *)malloc((slen + size) * sizeof(char));

    p = (unsigned char *)array;
    s = strcpy(s, conv(p[1]));
    s = strcat(strcat(s, "."), conv(p[0]));
    for (int i = 1; i < size; i++) {
        array += 1;
        p = (unsigned char *)array;
        s = strcat(strcat(s, "."), conv(p[1]));
        s = strcat(strcat(s, "."), conv(p[0]));
    }
    return s;
}


/* return a string with binary representation of inum */
char *int_to_bin(uint inum)
{
    size_t bits = sizeof(int) * CHAR_BIT;

    char * str = malloc((bits + 1) * sizeof(char));
    if(!str) return NULL;
    str[bits] = 0;

    // type punning because signed shift is implementation-defined
    uint32_t u = *(unsigned *) & inum;
    for(; bits--; u >>= 1)
        str[bits] = u & 1 ? '1' : '0';

    return str;
}

/* 
 * initialize read register array
 */
void init_rrega() {

    for(int i = 0; i < REG_SIZE; i++) {
        creg[i] = 0;
        ibreg[i] = 0;
        ireg[i] = 0;
        hreg[i] = 0;
    }
}

/* 
 * convert an array of uint16_t words with ASCCI characters to a string 
 */
char *words_to_str(const uint16_t *array, int length)
{
    char *s = malloc(length * sizeof(uint16_t));
    char *r = s;

    for (int i = 0; i < length; i++) {
        for (int j = 1; j >= 0; j--) {
            *s = *(((char *)array) + j);
            s += 1;
        }
        array += 1;
    }
    return r;
}

/*
 * Concatenate and invert 16bit words to 32bit (length = 2)
 * or 64bit (length = 4) which are stored in array. Returns
 * a 64bit integer.
 */
uint64_t concat_inv16(const uint16_t *array, int length)
{
    uint64_t inum = 0;
    uint16_t *p;

    p = (uint16_t *)&inum;
    for (int i = 0; i < length; i++) {
        *(p + length - i - 1) |= array[i];
    }

    return inum;
}

/*
 * convert a decimal integer to string
 */
char *int_to_str(int inum)
{
    int sz = snprintf(NULL, 0, "%d", inum);
    char *key = (char *) malloc((sz + 1) * sizeof(char));

    snprintf(key, sz + 1, "%d", inum);
    return key;
}

/*
 * convert a hex integer to string
 */
char *hex_to_str(int xnum)
{
    int sz = snprintf(NULL, 0, "%d", xnum);
    char *key = (char *) malloc((sz + 1) * sizeof(char));

    snprintf(key, sz + 1, "%x", xnum);
    return key;
}


/* 
 * create a new modbus context 
 */
modbus_t *modbus_new(char *port, serconf_t sc)
{
    modbus_t *mb = NULL;

    /* check port */
    if (port != NULL) {
        char *sp;

        /* open serial port */
        if (strstr(port, "/dev/tty") != NULL) {
            mb = modbus_new_rtu(port, sc.baud, sc.prty, sc.dbit, sc.sbit);
            if (mb == NULL) {
                printf("modbus_new_rtu: Unable to open serial port context\n");
            }

        /* open ip:port */
        } else if ((sp = strchr(port, ':')) != NULL) {
            char *tcp_port = malloc(sizeof(sp));
            strcpy(tcp_port, sp + 1);
            *sp = '\0';
            mb = modbus_new_tcp(port, (int )strtoul(tcp_port, NULL, 10));
            free(tcp_port);
        } else {
            mb = modbus_new_tcp(port, 502);
            if (mb == NULL) {
                printf("modbus_new_tcp: Unable to connect to %s\n", port);
            }
        }
    }
    return mb;
}

/* 
 * initialize modbus connection 
 */
modbus_t *modbus_init(char *port, serconf_t sc, int id)
{
    modbus_t *mb;       /* modbus context */
    int rval = -1;

    /* open modbus port and create a new modbus context */
    mb = modbus_new(port, sc);
    if (mb == NULL) {
        printf("modbus_new: Unable to connect to %s\n", port);
        return NULL;
    }

    /* set slave ID */
    rval = modbus_set_slave(mb, id);	/* slave ID */
    if (rval < 0) {
        modbus_free(mb);
        printf("modbus_set_slave: Invalid modbus slave ID %d\n", id);
        return NULL;
    }

    /* connect to modbus device  */
    rval = modbus_connect(mb);
    if (rval < 0) {
        modbus_free(mb);
        printf( "modbus_connect: unable to connect: %s\n", modbus_strerror(errno));
        return NULL;
    }

    /* set modbus response time out to 200ms */
    rval = modbus_set_response_timeout(mb, 0, 200000);
    if (rval < 0) {
        modbus_free(mb);
        printf("modbus_set_response_timeout: error(%s)", modbus_strerror(errno));
        exit(EXIT_FAILURE);
    }

    /* set modbus response time out to 200ms */
    rval = modbus_set_byte_timeout(mb, 0, 200000);
    if (rval < 0) {
        modbus_free(mb);
        printf("modbus_set_byte_timeout: error(%s)", modbus_strerror(errno));
        exit(EXIT_FAILURE);
    }
    return mb;
}

/* 
 * Initialize device register list 
 */
int init_drlist(dvlist_t **lst)
{
    DIR* FD;
    struct dirent* in_file;
    dreg_t *drarr = NULL;
    int cnt;

    /* Scanning the devices' directory */
    if (NULL == (FD = opendir ("./regs"))) {
        fprintf(stderr, "Error: Failed to open devices' directory\n");
        exit(EXIT_FAILURE);
    }
    cnt = 0;
    while ((in_file = readdir(FD))) {
        /* On linux/Unix we don't want current and parent directories */
        if (!strcmp (in_file->d_name, ".")) {
            continue;
        }
        if (!strcmp (in_file->d_name, "..")) {
            continue;
        }
        cnt++;
    }

    /* allocate memory for device list */
    *lst = (dvlist_t *)malloc(cnt * sizeof(dvlist_t));

    /* allocate memory for device register array of size cnt */
    drarr = (dreg_t *)malloc(sizeof(dreg_t));
    (*lst)->regs = drarr;

    return cnt;
}

/* 
 * read device and register info 
 */
void read_dreg(dvlist_t *lst)
{
    /* configuration vars */
    config_t cfg;
    config_setting_t *regs;
    const char *str;
    char *path;

    /* directory operations vars */
    DIR* FD;
    struct dirent* in_file;

    dvlist_t *dvl = lst;

    /* Scanning the devices' directory */
    if (NULL == (FD = opendir ("./regs"))) {
        fprintf(stderr, "Error: Failed to open devices' directory\n");
        exit(EXIT_FAILURE);
    }
    while ((in_file = readdir(FD))) {
        /* On linux/Unix we don't want current and parent directories */
        if (!strcmp (in_file->d_name, ".")) {
            continue;
        }
        if (!strcmp (in_file->d_name, "..")) {
            continue;
        }

        path = (char *)malloc((strlen("./regs/") + strlen(in_file->d_name) + 1) * sizeof(char));
        if (!path) {
            fprintf(stderr, "malloc failed: insufficient memory!\n");
            exit(EXIT_FAILURE);
        }
        strcpy(path, "./regs/");
        strcat(path, in_file->d_name);

        config_init(&cfg);

        /* Read the file. If there is an error, report it and exit. */
        if (!config_read_file(&cfg, path)) {
            fprintf(stderr, "%s:%d - %s\n", config_error_file(&cfg),
                    config_error_line(&cfg), config_error_text(&cfg));
            config_destroy(&cfg);
            exit(EXIT_FAILURE);
        }

        /* Get the device manufacturer */
        if (config_lookup_string(&cfg, "device.manfc", &str)) {
            //printf("Device mfr: %s\n", str);
            dvl->manfc = malloc((strlen(str) + 1) * sizeof(char));
            strcpy(dvl->manfc, str);
        } else {
            fprintf(stderr, "No 'device manfc' in configuration file.\n");
        }

        /* Get the device type */
        if (config_lookup_string(&cfg, "device.type", &str)) {
            //printf("Device type: %s\n", str);
            dvl->type = malloc((strlen(str) + 1) * sizeof(char));
            strcpy(dvl->type, str);
        } else {
            fprintf(stderr, "No 'device type' in configuration file.\n");
        }

        /* Get the device model */
        if (config_lookup_string(&cfg, "device.model", &str)) {
            //printf("Device model: %s\n", str);
            dvl->model = malloc((strlen(str) + 1) * sizeof(char));
            strcpy(dvl->model, str);
        } else {
            fprintf(stderr, "No 'device model' in configuration file.\n");
        }

        /* Get the zero based addressing configuration */
        if (config_lookup_int(&cfg, "device.zba", &dvl->zba) == 0) {
            fprintf(stderr, "No 'device zba' in configuration file.\n");
            config_destroy(&cfg);
            exit(EXIT_FAILURE);
        }

        /* Output a list of all books in the inventory. */
        regs = config_lookup(&cfg, "regs");
        if (regs != NULL) {
            int cnt = config_setting_length(regs);
            if (cnt != 0) {
                dvl->regs = (dreg_t *)malloc(cnt * sizeof(dreg_t));
                dreg_t *r = dvl->regs;
                dvl->nor = cnt;
                for (int i = 0; i < cnt; ++i) {
                    config_setting_t *reg = config_setting_get_elem(regs, i);
                    const char *name;
                    const char *desc;
                    const char *range;
                    const char *engu;
                    const char *access;
                    if (!(config_setting_lookup_int(reg, "num", &r->num) &&
                    config_setting_lookup_int(reg, "addr", &r->addr) &&
                    config_setting_lookup_int(reg, "len", &r->len) &&
                    config_setting_lookup_int(reg, "type", &r->type) &&
                    config_setting_lookup_string(reg, "name", &name) &&
                    config_setting_lookup_string(reg, "descr", &desc) &&
                    config_setting_lookup_string(reg, "range", &range) &&
                    config_setting_lookup_float(reg, "scale", &r->scale) &&
                    config_setting_lookup_int(reg, "print", &r->prfmt) &&
                    config_setting_lookup_string(reg, "engu", &engu) &&
                    config_setting_lookup_string(reg, "access", &access))) {
                        dvl->nor--;
                        continue;
                    }
                    r->name = (char *)malloc((strlen(name) + 1) * sizeof(char));
                    strcpy(r->name, name);
                    r->desc = (char *)malloc((strlen(desc) + 1) * sizeof(char));
                    strcpy(r->desc, desc);
                    r->range = (char *)malloc((strlen(range) + 1) * sizeof(char));
                    strcpy(r->range, range);
                    r->engu = (char *)malloc((strlen(engu) + 1) * sizeof(char));
                    strcpy(r->engu, engu);
                    r->acc = (char *)malloc((strlen(access) + 1) * sizeof(char));
                    strcpy(r->acc, access);
                    if (r->addr == 0) {

                        /* calculate the digits of the register number and allocate the proper memory size */
                        char *hexreg = NULL;
                        if (r->num != 0) {
                            hexreg = (char *)malloc(sizeof(char) * (int) (log10(r->num) + 1));
                        } else {
                            hexreg = (char *)malloc(sizeof(char));
                        }
                        sprintf(hexreg, "%x", r->num);
                        int rnum = (int )(0x0 + strtoul(hexreg, NULL, 16));
                        switch (r->type) {
                            case COIL:
                                r->addr = 0x0 + rnum;
                                break;
                            case INPUT_B:
                                r->addr = 0x10000 + rnum;
                                break;
                            case INPUT_R:
                                r->addr = 0x30000 + rnum;
                                break;
                            case HOLDING:
                                r->addr = 0x40000 + rnum;
                                break;
                            default:
                                printf("Invalid register type\n");
                        }
                        free(hexreg);
                    }
                    r++;
                }
            }
        }
        config_destroy(&cfg);
        free(path);
        dvl++;
    }
}

/* 
 * print supported devices' information
 */
void print_dev_info(dvlist_t *lst, int sz)
{
    dvlist_t *dvl = lst;

    printf("Supported devices:\n");
    printf("%-3s %-12s %-12s %-15s %-4s\n", "NUM", "TYPE", "MANUFACTURER", "MODEL", "REGS");
    int cnt = 0;
    while (cnt < sz) {
        printf("%-3d %-12s %-12s %-15s %-4d\n", cnt + 1, dvl->type, dvl->manfc, dvl->model, dvl->nor);
        cnt++;
        dvl++;
    }
}

/*
 * print supported device registers' information
 */
void print_dev_reginfo(dvlist_t *lst, int num, int nor)
{
    dvlist_t *dvl = lst;
    dreg_t *regs = dvl[num].regs;

    printf("%s %s %s\n", dvl[num].manfc, dvl[num].model, dvl[num].type);
    printf("%-5s %-12s %-35s %-90s %-3s %-10s %-7s %-12s %-3s\n", "NUM", "ADDRESS", "NAME",
           "DESCRIPTION", "LEN", "RANGE", "SCALE", "ENGU", "ACC");
    int cnt = 0;
    while (cnt < nor) {
        printf("%-5d 0x%-10x %-35s %-90s %-3d %-10s %-7.2f %-12s %-3s\n", regs->num, regs->addr, regs->name,
                regs->desc, regs->len, regs->range, regs->scale, regs->engu, regs->acc);
        cnt++;
        regs++;
    }
}

/* print the program command line usage */
void usage(char *pname)
{
    printf("Usage: %s [OPTIONS]...\n", pname);
    printf("    -p <port>      device port can be either a file or an IP address (default: /dev/ttyUSB0)\n");
    printf("                   example: /dev/tty<PORT>, 192.0.12.3, 192.168.1.2:1502 (default TCP port 502)\n");
    printf("    -u <baud_rate> serial port baud rate (default 9600)\n");
    printf("    -y <parity>    serial port parity (N:none O:odd E:even M:mark default N)\n");
    printf("    -o <stop_bit>  serial port stop bit (default 1)\n");
    printf("    -d <data_bits> serial port data bits (default 8)\n");
    printf("    -i <slave_id>  modbus slave device id (default 1)\n");
    printf("    -z             modbus zero based addressing (address|register - 1)\n");
    printf("    -a <address> | memory map address (default address 0x0)\n");
    printf("       <reg_num> | register number (default 1) if -g has been specified\n");
    printf("       <v,v,v,v>   comma separated values of addresses or registers e.g. -a0x40032,0x40101,0x4078\n");
    printf("                   example: modio -p/dev/ttyS0 -a0x40032,0x40101,0x4078 -r -t3\n");
    printf("    -r             read data from memory\n");
    printf("    -w <data>      write data to address or register\n");
    printf("    -l <size>      length of read count from address or register (default 1)\n");
    printf("                   length is defined in words or registers and word size depends on register type\n");
    printf("                   example: modio -p/dev/ttyS0 -a0x40078 -l3 -r -t3 reads 3 16bit registers\n");
    printf("                   starting from address 0x40078\n");
    printf("    -g             read (write) data from (to) register \n");
    printf("    -t <reg_type>  register type (0:COIL 1:INPUT_BIT 2:INPUT_REG 3:HOLDING, default 0)\n");
    printf("    -b             print binary values (applicable to INPUT_REG and HOLDING registers)\n");
    printf("    -x             print hex values (applicable INPUT_REG and HOLDING registers)\n");
    printf("    -s             print values as strings (applicable to INPUT_REG and HOLDING registers)\n");
    printf("    -f <format>    print values as strings with '.' separated bytes (1:decimal 2:hex)\n");
    printf("                   applicable to INPUT_REG and HOLDING registers\n");
    printf("    -n <int_type>  print values as integers (1:short 2:int 3:long)\n");
    printf("                   applicable to INPUT_REG and HOLDING registers \n");
    printf("    -v <dev_id>    print verbose register info for selected device \n");
    printf("    -c             print supported devices\n");
    printf("    -e <dev_id>    print supported device registers' info\n");
    printf("    -j <dev_id>    jot device registers' info on screen\n");
    printf("    -h             help, print usage\n");
}

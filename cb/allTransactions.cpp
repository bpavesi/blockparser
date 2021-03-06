
// Dump all transactions affecting a specific address

#include <time.h>
#include <util.h>
#include <vector>
#include <common.h>
#include <errlog.h>
#include <option.h>
#include <rmd160.h>
#include <string.h>
#include <callback.h>

static uint8_t emptyKey[kRIPEMD160ByteSize] = { 0x52 };
typedef GoogMap<Hash160, int, Hash160Hasher, Hash160Equal>::Map AddrMap;

struct AllTransactions:public Callback
{
    bool csv;
    optparse::OptionParser parser;

    uint64_t adds;
    uint64_t subs;
    uint64_t nbTX;
    uint64_t bTime;

    AllTransactions()
    {
        parser
            .usage("[options]")
            .version("")
            .description("dump all transactions for all addresses")
            .epilog("")
        ;
        parser
            .add_option("-c", "--csv")
            .action("store_true")
            .set_default(false)
            .help("produce CSV-formatted output instead column-formatted")
        ;
    }

    virtual const char                   *name() const         { return "allTransactions"; }
    virtual const optparse::OptionParser *optionParser() const { return &parser;        }
    virtual bool                         needTXHash() const    { return true;           }

    virtual void aliases(
        std::vector<const char*> &v
    ) const
    {
        v.push_back("txs");
        v.push_back("book");
        v.push_back("tally");
    }

    virtual int init(
        int  argc,
        const char *argv[]
    )
    {
        adds = 0;
        subs = 0;
        nbTX = 0;

        optparse::Values &values = parser.parse_args(argc, argv);
        csv = values.get("csv");

        return 0;
    }

    void move(
        const uint8_t *script,
        uint64_t      scriptSize,
        const uint8_t *txHash,
        uint64_t       value,
        bool           add,
        const uint8_t *downTXHash = 0
    )
    {
        uint8_t addrType[3];
        uint160_t pubKeyHash;
        int type = solveOutputScript(pubKeyHash.v, script, scriptSize, addrType);
        if(unlikely(type<0)) return;

        if(true) {


            if(csv) {
                printf("%6" PRIu64 ", \"", bTime/86400 + 25569);
                showHex(pubKeyHash.v, kRIPEMD160ByteSize, false);
                printf("\", \"");
                showHex(downTXHash ? downTXHash : txHash);
                printf(
                    "\",%17.08f\n",
                    (add ? 1e-8 : -1e-8)*value
                );
            } else {

                struct tm gmTime;
                time_t blockTime = bTime;
                gmtime_r(&blockTime, &gmTime);

                char timeBuf[256];
                asctime_r(&gmTime, timeBuf);

                size_t sz =strlen(timeBuf);
                if(0<sz) timeBuf[sz-1] = 0;

                printf("    %s    ", timeBuf);
                showHex(pubKeyHash.v, kRIPEMD160ByteSize, false);

                printf("    ");
                showHex(downTXHash ? downTXHash : txHash);

                printf(
                    " %c %24.08f\n",
                    add ? '+' : '-',
                    value*1e-8
                );
            }

            (add ? adds : subs) += value;
            ++nbTX;
        }
    }

    virtual void endOutput(
        const uint8_t *p,
        uint64_t      value,
        const uint8_t *txHash,
        uint64_t      outputIndex,
        const uint8_t *outputScript,
        uint64_t      outputScriptSize
    )
    {
        move(
            outputScript,
            outputScriptSize,
            txHash,
            value,
            true
        );
    }

    virtual void edge(
        uint64_t      value,
        const uint8_t *upTXHash,
        uint64_t      outputIndex,
        const uint8_t *outputScript,
        uint64_t      outputScriptSize,
        const uint8_t *downTXHash,
        uint64_t      inputIndex,
        const uint8_t *inputScript,
        uint64_t      inputScriptSize
    )
    {
        move(
            outputScript,
            outputScriptSize,
            upTXHash,
            value,
            false,
            downTXHash
        );
    }

    virtual void startBlock(
        const Block *b,
        uint64_t
    )
    {
        const uint8_t *p = b->data;
        SKIP(uint32_t, version, p);
        SKIP(uint256_t, prevBlkHash, p);
        SKIP(uint256_t, blkMerkleRoot, p);
        LOAD(uint32_t, blkTime, p);
        bTime = blkTime;
    }

    virtual void start(
        const Block *,
        const Block *
    )
    {
        if(csv) {
            printf(
                "\"Time\","
                " \"Address\","
                "                                  \"TXId\","
                "                                                                   \"TXAmount\""
                "\n"
            );
        }
        else {
            info("Dumping all transactions");
            printf("    Time (GMT)                  Address                                     Transaction                                                                    Amount\n");
            printf("    ===================================================================================================================================================================\n");
        }
    }

    virtual void wrapup()
    {
        if(false==csv) {
            printf("    ===================================================================================================================================================================\n");

            info(
                "\n"
                "    transactions  = %" PRIu64 "\n"
                "\n",
                nbTX
            );
        }
    }
};

static AllTransactions alltransactions;


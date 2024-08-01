
extern int TBSCLI(int argc, const char* argv[]);

int main(int argc, const char* realArgv[])
{
    const const char* argv[] = {
        realArgv[0],
        "-f", "C:\\Users\\Wing\\Desktop\\prueva.bin",
        "-p", "AA AA AA AA",
        "--quiet"
    };

    return TBSCLI(sizeof(argv) / sizeof(argv[0]), argv);
}
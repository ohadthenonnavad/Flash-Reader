savedcmd_spi_reader.mod := printf '%s\n'   spi_reader.o | awk '!x[$$0]++ { print("./"$$0) }' > spi_reader.mod

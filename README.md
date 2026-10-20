# CoinBrowser

A desktop tool for [Freqtrade](https://www.freqtrade.io/). It shows what every
pair on an exchange is doing, filters that list down to the ones worth trading,
and writes the result as a pair list Freqtrade can use. A portfolio simulator is
built in for trying an idea without spending anything.

**No API key, for anything.** Prices come from the exchange itself and market
caps from CoinGecko, neither of which asks you to sign up.

![The main window](https://github.com/QTinman/CoinBrowser/blob/main/screencap.png)

## Where the numbers come from

| | source | needs a key |
|---|---|---|
| price, volume, 24h change | the selected exchange | no |
| market cap, rank, 1h and 7d change | CoinGecko | no |

Binance and KuCoin are supported for prices. One request returns every symbol
they trade, so the price shown is the one that exchange would actually fill at
rather than an average across venues. If CoinGecko is unreachable the prices are
still used without it.

CoinGecko is asked for its top 1000 by market cap, in four pages. A coin outside
that list has no cap, rank or 1h/7d figure to report, so **those columns read 0
and its rank is numbered above 1000** — on Binance USDT that is about half of the
703 tradeable pairs, all of them small. Price, volume and the 24h change always
come from the exchange and are always real.

## The columns

Most are self-explanatory. The ones that need a word:

- **Rank** — market cap rank from CoinGecko. Anything above 1000 is a coin
  CoinGecko does not rank, numbered after the ones it does so that no two coins
  share a number.
- **DB +/- %** — how far the price has moved since the table selected in the
  **Table** dropdown. 0 means that coin was not in that table.
- **DB rank move** — how far it has moved up or down the rankings since then.
  `-` means it was not in that table, so there is nothing to compare against.
- **db/json time** — the time inside the compared table, and the time of the
  prices now on screen.

The line under the table says exactly what the comparison is against, for example
`compared against 433 coins from 18.10.2026 09:51 (USDT_18-10-2026_09-51)`.
Dates follow the machine's own convention, so 18/10 and 10/18 each read correctly
where they are read.

Click any header to sort. Prices keep about five significant digits whatever the
stake coin, so a pair quoted in BTC at 0.0000158 reads as 0.0000158 rather than
rounding away to nothing.

## Tables

Every update writes a new table into `coinhistory.db`, named for the moment it
was made. The **Table** dropdown lists them newest first, labelled by date, and
whichever is selected is what the comparison columns measure against.

Tables are never deleted. *Table age in hours shown* in Settings controls how far
back the dropdown goes; older tables stay in the file.

## Pair lists

Pairs are read from `raw_<exchange>.txt`, made with Freqtrade itself:

    docker-compose run --rm freqtrade list-pairs --exchange binance > raw_binance.txt

Copy the table from that output into the file. It goes stale: a list taken in
2022 had 147 of its 335 USDT pairs delisted and was missing 323 added since, so
re-make it when the pair list looks wrong.

The leftmost toolbar button writes the filtered coins back out as a pair list for
Freqtrade, into the *Out file path* from Settings.

## Portfolio simulator

![The portfolio simulator](https://github.com/QTinman/CoinBrowser/blob/main/simulator.png)

Fictional buys and sells against live prices, to see how an idea would have gone.
It starts with 1000 to play with, and its coin list is the pair file the main
window writes — created for you on a first run, or chosen in Settings.

Press **Get price** before buying; it fetches the last five-minute close for the
selected coin. The database button refreshes the current price of everything held.

## Settings

![Settings](https://github.com/QTinman/CoinBrowser/blob/main/settings.png)

- **Json Path** — where the fetched price data is written and read.
- **In file path** — where `raw_<exchange>.txt` is read from.
- **Out file path** — where the Freqtrade pair list is written.
- **Reports path**, **Reports** — optional CSV of every update.
- **Crypto invest file**, **Balance** — the simulator's coin list and stake.
- **Price update delay mSec** — how long to wait between price requests when
  refreshing the simulator. Raise it if prices fail to update.
- **json auto update in min**, **Auto upd.** — fetch prices on a timer.
- **Table age in hours shown** — how far back the Table dropdown lists.
- **Blacklist** — coins to leave out, per exchange.
- **Stake coin** — what pairs are quoted in. The price column follows it.

## Building

Needs Qt 6 (Qt 5 also resolves) with the Widgets, Sql and Network modules, plus
OpenSSL.

    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build

On Fedora: `sudo dnf install qt6-qtbase-devel cmake gcc-c++ openssl-devel`.
Debian and Ubuntu: `qt6-base-dev libqt6sql6-sqlite cmake g++ libssl-dev`. The
SQLite driver is a separate package on Debian, and the program cannot open its
database without it.

On Windows with the Qt MinGW kit, point CMake at Qt's own OpenSSL:

    cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release ^
      -DCMAKE_PREFIX_PATH=C:/Qt/6.5.2/mingw_64 ^
      -DOPENSSL_ROOT_DIR=C:/Qt/Tools/OpenSSLv3/Win_x64

To package it, run `windeployqt` **without** `--release`: it misreads MinGW
plugins as debug builds and silently deploys none of them, leaving an executable
that cannot find its platform plugin. Copy `libssl-3-x64.dll` and
`libcrypto-3-x64.dll` in as well, along with the Visual C++ runtime they need.

## Downloads

Built packages for Windows and Linux are on the
[releases page](https://github.com/QTinman/CoinBrowser/releases). The Windows zip
carries everything it needs; the Linux archive uses the Qt already installed on
the system.

## Donate

If you find this program useful:

    BTC  bc1qt4jv89m5gy62e64j9kwyhu7n65s0yhp4nqayag
    ETH  0x9129B88AA1fEE2545d1123e71e9Fa2c43C075979
    SOL  3s4nqsu2WiPFrXSmxKY7U3zX4RtKCEgczaNkVBwcgdXS
    ZEC  t1fb8QAsRdNT2sF6LXoYsHSmHLv18UpPW9D

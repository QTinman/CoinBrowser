# CoinBrowser
CoinBrowser is a tool for Freqtrade where the program writes pairs into text file to be used with spesific exchange, these pairs file is also used in the simulator.

Prices come from the selected exchange directly - Binance or KuCoin - in one
request and with no API key. Market cap, rank and the 1h/7d changes come from
CoinGecko, which also needs no key; an exchange does not know them. If
CoinGecko is unreachable the prices are still used without them.

Coins outside CoinGecko's top 1000 by market cap carry no cap, rank or 1h/7d
figure, so those three columns read 0 for the long tail. Price, volume and the
24h change always come from the exchange and are always real.

CoinMarketCap is no longer used and the API key setting is gone.

If database does not exist upon startup the program creates coinhistory.db database from exiting json file.

The database is updated into own table from existing json file right before new json file is downloaded.

Database can then be selected from droppdown in mainwindow.


![Welcome screen](https://github.com/QTinman/CoinBrowser/blob/main/screencap.png)


The txt files starting with "raw_" is created with command "docker-compose run --rm freqtrade list-pairs --exchange binance" and the table from the output copied to new file, example raw_binance.txt.

That file goes stale: a list taken in 2022 had 147 of its 335 USDT pairs
delisted and was missing 323 that had since been added. Re-make it when the
pair list looks wrong.

Invest simulator lets you do fictional crypto buys.

To get crypto in simulator dropdown list do write to file in mainwindow and point to that file in settings.

![Welcome screen](https://github.com/QTinman/CoinBrowser/blob/main/simulator.png)

Crypto invest simulator

![Welcome screen](https://github.com/QTinman/CoinBrowser/blob/main/settings.png)

Typical settings.

# CoinBrowser
CoinBrowser is a tool for Freqtrade where the program writes pairs into text file to be used with spesific exchange.

Data for this program is from CoinMarkedCap API. API key is needed for downloading the json file.
The program creates coinhistory.db database if it is missing and uses the data from exiting json file.
The database can be updated by checking UpdateDB and will it then upgrade from existing json file.
Typically it is good to have the database few hours older than the json file as the DB should be few hours older for some calculations.

#pragma once
#include "piaabo/dutils.h"

RUNTIME_WARNING("(binance_websocket.h)[] handle websocket api key revocation\n");
RUNTIME_WARNING("(binance_websocket.h)[] handle websocket 24 h reconection\n");
RUNTIME_WARNING("(binance_websocket.h)[] impement rest codes verification 200, 4XX, 400, 403, 409, 418, 429, 5XX\n")
RUNTIME_WARNING("(binance_websocket.h)[] implement ratelimit verfication\n");
RUNTIME_WARNING("(binance_websocket.h)[] implement two key managment system, one for account data and other for trading\n");
RUNTIME_WARNING("(binance_websocket.h)[] if noted that bot is trying to do high frequency, it's important to manage recWindow\n");

# Methods

ping
time
exchangeInfo
depth
trades.recent
trades.historical
trades.aggregate
klines
uiKlines
avgPrice
ticker.24hr
ticker.tradingDay
ticker
ticker.price
ticker.book
session.logon
session.status
session.logout
order.place
order.test
order.status
order.cancel
order.cancelReplace
openOrders.status
openOrders.cancelAll
orderList.place
orderList.place.oco
orderList.place.oto
orderList.place.otoco
orderList.status
orderList.cancel
openOrderLists.status
sor.order.place
sor.order.test
account.status
account.rateLimits.orders
allOrders
allOrderLists
myTrades
myPreventedMatches
myAllocations
account.commission
userDataStream.start
userDataStream.ping
userDataStream.stop

#pragma once
#include "piaabo/risk_metric.h"
RUNTIME_WATNING("(risk_metric.h)[] consider adding the kelly rule.\n");
RUNTIME_WATNING("(risk_metric.h)[] remmember the fat tail distribution of the risk in finantial markets (risk is not a normal distribution, here the tails are fat) James S..\n");
RUNTIME_WATNING("(risk_metric.h)[] there has to be something more precise than probability.\n");
RUNTIME_WARNING("(risk_metric.h)[] consider herarchical risk parity (HRP) and hearachical clustering portfolios (HCP). \n");
THROW_COMPILE_TIME_ERROR("Not implemented risk_metric.h\n");

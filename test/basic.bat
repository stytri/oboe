@call %1 hello-world  %2 %3 %4 %5 %6 %7 %8 %9
@for %%f IN (basic-*.oboe) DO @ call %1 %%~nf %2 %3 %4 %5 %6 %7 %8 %9
@call %1 ISO-week-number 2020 01 05 2020 01 06 2020 01 12 2020 01 13 2020 12 13 2020 12 14
@call %1 ordinal 100 129
@call %1 vectest

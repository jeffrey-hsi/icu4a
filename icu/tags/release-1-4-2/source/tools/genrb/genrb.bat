@echo off
rem Added argument checking, Vladimir Weinsten - IBM Corp., 10/25/99

if "%1"=="" goto :error

if "%2"=="" goto :error

if "%ICU_DATA%"=="" set ICU_DATA=%2\icu\data\

%1\genrb %2\icu\data\default.txt
%1\genrb %2\icu\data\index.txt
%1\genrb %2\icu\data\ar.txt
%1\genrb %2\icu\data\ar_AE.txt
%1\genrb %2\icu\data\ar_BH.txt
%1\genrb %2\icu\data\ar_DZ.txt
%1\genrb %2\icu\data\ar_EG.txt
%1\genrb %2\icu\data\ar_IQ.txt
%1\genrb %2\icu\data\ar_JO.txt
%1\genrb %2\icu\data\ar_KW.txt
%1\genrb %2\icu\data\ar_LB.txt
%1\genrb %2\icu\data\ar_LY.txt
%1\genrb %2\icu\data\ar_MA.txt
%1\genrb %2\icu\data\ar_OM.txt
%1\genrb %2\icu\data\ar_QA.txt
%1\genrb %2\icu\data\ar_SA.txt
%1\genrb %2\icu\data\ar_SD.txt
%1\genrb %2\icu\data\ar_SY.txt
%1\genrb %2\icu\data\ar_TN.txt
%1\genrb %2\icu\data\ar_YE.txt
%1\genrb %2\icu\data\be.txt
%1\genrb %2\icu\data\be_BY.txt
%1\genrb %2\icu\data\bg.txt
%1\genrb %2\icu\data\bg_BG.txt
%1\genrb %2\icu\data\ca.txt
%1\genrb %2\icu\data\ca_ES.txt
%1\genrb %2\icu\data\ca_ES_EURO.txt
%1\genrb %2\icu\data\cs.txt
%1\genrb %2\icu\data\cs_CZ.txt
%1\genrb %2\icu\data\da.txt
%1\genrb %2\icu\data\da_DK.txt
%1\genrb %2\icu\data\de.txt
%1\genrb %2\icu\data\de_AT.txt
%1\genrb %2\icu\data\de_AT_EURO.txt
%1\genrb %2\icu\data\de_CH.txt
%1\genrb %2\icu\data\de_DE.txt
%1\genrb %2\icu\data\de_DE_EURO.txt
%1\genrb %2\icu\data\de_LU.txt
%1\genrb %2\icu\data\de_LU_EURO.txt
%1\genrb %2\icu\data\el.txt
%1\genrb %2\icu\data\el_GR.txt
%1\genrb %2\icu\data\en.txt
%1\genrb %2\icu\data\en_AU.txt
%1\genrb %2\icu\data\en_CA.txt
%1\genrb %2\icu\data\en_GB.txt
%1\genrb %2\icu\data\en_IE.txt
%1\genrb %2\icu\data\en_IE_EURO.txt
%1\genrb %2\icu\data\en_NZ.txt
%1\genrb %2\icu\data\en_US.txt
%1\genrb %2\icu\data\en_ZA.txt
%1\genrb %2\icu\data\es.txt
%1\genrb %2\icu\data\es_AR.txt
%1\genrb %2\icu\data\es_BO.txt
%1\genrb %2\icu\data\es_CL.txt
%1\genrb %2\icu\data\es_CO.txt
%1\genrb %2\icu\data\es_CR.txt
%1\genrb %2\icu\data\es_DO.txt
%1\genrb %2\icu\data\es_EC.txt
%1\genrb %2\icu\data\es_ES.txt
%1\genrb %2\icu\data\es_ES_EURO.txt
%1\genrb %2\icu\data\es_GT.txt
%1\genrb %2\icu\data\es_HN.txt
%1\genrb %2\icu\data\es_MX.txt
%1\genrb %2\icu\data\es_NI.txt
%1\genrb %2\icu\data\es_PA.txt
%1\genrb %2\icu\data\es_PE.txt
%1\genrb %2\icu\data\es_PR.txt
%1\genrb %2\icu\data\es_PY.txt
%1\genrb %2\icu\data\es_SV.txt
%1\genrb %2\icu\data\es_UY.txt
%1\genrb %2\icu\data\es_VE.txt
%1\genrb %2\icu\data\et.txt
%1\genrb %2\icu\data\et_EE.txt
%1\genrb %2\icu\data\fi.txt
%1\genrb %2\icu\data\fi_FI.txt
%1\genrb %2\icu\data\fi_FI_EURO.txt
%1\genrb %2\icu\data\fr.txt
%1\genrb %2\icu\data\fr_BE.txt
%1\genrb %2\icu\data\fr_BE_EURO.txt
%1\genrb %2\icu\data\fr_CA.txt
%1\genrb %2\icu\data\fr_CH.txt
%1\genrb %2\icu\data\fr_FR.txt
%1\genrb %2\icu\data\fr_FR_EURO.txt
%1\genrb %2\icu\data\fr_LU.txt
%1\genrb %2\icu\data\fr_LU_EURO.txt
%1\genrb %2\icu\data\hr.txt
%1\genrb %2\icu\data\hr_HR.txt
%1\genrb %2\icu\data\hu.txt
%1\genrb %2\icu\data\hu_HU.txt
%1\genrb %2\icu\data\is.txt
%1\genrb %2\icu\data\is_IS.txt
%1\genrb %2\icu\data\it.txt
%1\genrb %2\icu\data\it_CH.txt
%1\genrb %2\icu\data\it_IT.txt
%1\genrb %2\icu\data\it_IT_EURO.txt
%1\genrb %2\icu\data\iw.txt
%1\genrb %2\icu\data\iw_IL.txt
%1\genrb %2\icu\data\ja.txt
%1\genrb %2\icu\data\ja_JP.txt
%1\genrb %2\icu\data\ko.txt
%1\genrb %2\icu\data\ko_KR.txt
%1\genrb %2\icu\data\lt.txt
%1\genrb %2\icu\data\lt_LT.txt
%1\genrb %2\icu\data\lv.txt
%1\genrb %2\icu\data\lv_LV.txt
%1\genrb %2\icu\data\mk.txt
%1\genrb %2\icu\data\mk_MK.txt
%1\genrb %2\icu\data\nl.txt
%1\genrb %2\icu\data\nl_BE.txt
%1\genrb %2\icu\data\nl_BE_EURO.txt
%1\genrb %2\icu\data\nl_NL.txt
%1\genrb %2\icu\data\nl_NL_EURO.txt
%1\genrb %2\icu\data\no.txt
%1\genrb %2\icu\data\no_NO.txt
%1\genrb %2\icu\data\no_NO_NY.txt
%1\genrb %2\icu\data\pl.txt
%1\genrb %2\icu\data\pl_PL.txt
%1\genrb %2\icu\data\pt.txt
%1\genrb %2\icu\data\pt_BR.txt
%1\genrb %2\icu\data\pt_PT.txt
%1\genrb %2\icu\data\pt_PT_EURO.txt
%1\genrb %2\icu\data\ro.txt
%1\genrb %2\icu\data\ro_RO.txt
%1\genrb %2\icu\data\ru.txt
%1\genrb %2\icu\data\ru_RU.txt
%1\genrb %2\icu\data\sh.txt
%1\genrb %2\icu\data\sh_YU.txt
%1\genrb %2\icu\data\sk.txt
%1\genrb %2\icu\data\sk_SK.txt
%1\genrb %2\icu\data\sl.txt
%1\genrb %2\icu\data\sl_SI.txt
%1\genrb %2\icu\data\sq.txt
%1\genrb %2\icu\data\sq_AL.txt
%1\genrb %2\icu\data\sr.txt
%1\genrb %2\icu\data\sr_YU.txt
%1\genrb %2\icu\data\sv.txt
%1\genrb %2\icu\data\sv_SE.txt
%1\genrb %2\icu\data\th.txt
%1\genrb %2\icu\data\th_TH.txt
%1\genrb %2\icu\data\tr.txt
%1\genrb %2\icu\data\tr_TR.txt
%1\genrb %2\icu\data\uk.txt
%1\genrb %2\icu\data\uk_UA.txt
%1\genrb %2\icu\data\vi.txt
%1\genrb %2\icu\data\vi_VN.txt
%1\genrb %2\icu\data\zh.txt
%1\genrb %2\icu\data\zh_CN.txt
%1\genrb %2\icu\data\zh_HK.txt
%1\genrb %2\icu\data\zh_TW.txt
rem RuleBasedTransliterator files
%1\genrb %2\icu\data\translit\expcon.txt
%1\genrb %2\icu\data\translit\kbdescl1.txt
%1\genrb %2\icu\data\translit\larabic.txt
%1\genrb %2\icu\data\translit\ldevan.txt
%1\genrb %2\icu\data\translit\lgreek.txt
%1\genrb %2\icu\data\translit\lhalfwid.txt
%1\genrb %2\icu\data\translit\lhebrew.txt
%1\genrb %2\icu\data\translit\lkana.txt
%1\genrb %2\icu\data\translit\lrussian.txt
%1\genrb %2\icu\data\translit\quotes.txt
%1\genrb %2\icu\data\translit\ucname.txt
rem Do the conversion for the test locales
%1\genrb %2\icu\source\test\testdata\default.txt
%1\genrb %2\icu\source\test\testdata\te.txt
%1\genrb %2\icu\source\test\testdata\te_IN.txt
goto :end

:error

echo call genrb with "Debug" or "Release" as the first argument
echo and the absolute path to the icu directory as the second.
echo for example, if you built the Debug version on icu, 
echo and the full path of icu is d:\mytools\icu then call
echo genrb Debug d:\mytools
echo the current directory must be the icu\source\tools\genrb directory with genrb.bat

:end

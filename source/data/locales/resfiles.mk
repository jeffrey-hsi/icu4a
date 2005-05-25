# *   Copyright (C) 1998-2005, International Business Machines
# *   Corporation and others.  All Rights Reserved.
# A list of txt's to build
# Note: 
#
#   If you are thinking of modifying this file, READ THIS. 
#
# Instead of changing this file [unless you want to check it back in],
# you should consider creating a 'reslocal.mk' file in this same directory.
# Then, you can have your local changes remain even if you upgrade or
# reconfigure ICU.
#
# Example 'reslocal.mk' files:
#
#  * To add an additional locale to the list: 
#    _____________________________________________________
#    |  GENRB_SOURCE_LOCAL =   myLocale.txt ...
#
#  * To REPLACE the default list and only build with a few
#     locale:
#    _____________________________________________________
#    |  GENRB_SOURCE = ar.txt ar_AE.txt en.txt de.txt zh.txt
#
#
# Generated by LDML2ICUConverter, from LDML source files. 

# Aliases which do not have a corresponding xx.xml file (see deprecatedList.xml)
GENRB_SYNTHETIC_ALIAS = in.txt in_ID.txt iw.txt iw_IL.txt\
 ja_JP_TRADITIONAL.txt no.txt no_NO.txt no_NO_NY.txt th_TH_TRADITIONAL.txt


# All aliases (to not be included under 'installed'), but not including root.
GENRB_ALIAS_SOURCE = $(GENRB_SYNTHETIC_ALIAS) sh.txt sh_YU.txt sr_Cyrl_YU.txt sr_Latn_YU.txt\
 sr_YU.txt zh_CN.txt zh_HK.txt zh_MO.txt zh_SG.txt\
 zh_TW.txt




# Ordinary resources
GENRB_SOURCE = af.txt af_ZA.txt am.txt am_ET.txt\
 ar.txt ar_AE.txt ar_BH.txt ar_DZ.txt ar_EG.txt ar_IN.txt\
 ar_IQ.txt ar_JO.txt ar_KW.txt ar_LB.txt ar_LY.txt\
 ar_MA.txt ar_OM.txt ar_QA.txt ar_SA.txt ar_SD.txt\
 ar_SY.txt ar_TN.txt ar_YE.txt be.txt be_BY.txt\
 bg.txt bg_BG.txt bn.txt bn_IN.txt ca.txt\
 ca_ES.txt cs.txt cs_CZ.txt cy.txt cy_GB.txt\
 da.txt da_DK.txt de.txt de_AT.txt de_BE.txt\
 de_CH.txt de_DE.txt de_LU.txt el.txt el_GR.txt\
 en.txt en_AU.txt en_BE.txt en_BW.txt en_CA.txt\
 en_GB.txt en_HK.txt en_IE.txt en_IN.txt en_MT.txt\
 en_NZ.txt en_PH.txt en_PK.txt en_SG.txt en_US.txt\
 en_US_POSIX.txt en_VI.txt en_ZA.txt en_ZW.txt eo.txt\
 es.txt es_AR.txt es_BO.txt es_CL.txt es_CO.txt\
 es_CR.txt es_DO.txt es_EC.txt es_ES.txt es_GT.txt\
 es_HN.txt es_MX.txt es_NI.txt es_PA.txt es_PE.txt\
 es_PR.txt es_PY.txt es_SV.txt es_US.txt es_UY.txt\
 es_VE.txt et.txt et_EE.txt eu.txt eu_ES.txt\
 fa.txt fa_AF.txt fa_IR.txt fi.txt fi_FI.txt\
 fo.txt fo_FO.txt fr.txt fr_BE.txt fr_CA.txt\
 fr_CH.txt fr_FR.txt fr_LU.txt ga.txt ga_IE.txt\
 gl.txt gl_ES.txt gu.txt gu_IN.txt gv.txt\
 gv_GB.txt he.txt he_IL.txt hi.txt hi_IN.txt\
 hr.txt hr_HR.txt hu.txt hu_HU.txt hy.txt hy_AM.txt\
 hy_AM_REVISED.txt id.txt id_ID.txt is.txt is_IS.txt\
 it.txt it_CH.txt it_IT.txt ja.txt ja_JP.txt\
 kk.txt kk_KZ.txt kl.txt kl_GL.txt kn.txt\
 kn_IN.txt ko.txt ko_KR.txt kok.txt kok_IN.txt\
 kw.txt kw_GB.txt lt.txt lt_LT.txt lv.txt\
 lv_LV.txt mk.txt mk_MK.txt ml.txt ml_IN.txt\
 mr.txt mr_IN.txt ms.txt ms_BN.txt ms_MY.txt\
 mt.txt mt_MT.txt nb.txt nb_NO.txt nl.txt\
 nl_BE.txt nl_NL.txt nn.txt nn_NO.txt om.txt\
 om_ET.txt om_KE.txt or.txt or_IN.txt pa.txt\
 pa_IN.txt pl.txt pl_PL.txt ps.txt ps_AF.txt\
 pt.txt pt_BR.txt pt_PT.txt ro.txt ro_RO.txt\
 ru.txt ru_RU.txt ru_UA.txt sk.txt sk_SK.txt\
 sl.txt sl_SI.txt so.txt so_DJ.txt so_ET.txt so_KE.txt\
 so_SO.txt sq.txt sq_AL.txt sr.txt sr_Cyrl.txt sr_Latn.txt\
 sv.txt sv_FI.txt sv_SE.txt sw.txt sw_KE.txt\
 sw_TZ.txt ta.txt ta_IN.txt te.txt te_IN.txt\
 th.txt th_TH.txt ti.txt ti_ER.txt ti_ET.txt\
 tr.txt tr_TR.txt uk.txt uk_UA.txt vi.txt\
 vi_VN.txt zh.txt zh_Hans.txt zh_Hans_CN.txt zh_Hans_SG.txt\
 zh_Hant.txt zh_Hant_HK.txt zh_Hant_MO.txt zh_Hant_TW.txt


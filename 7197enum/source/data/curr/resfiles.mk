# *   Copyright (C) 1998-2012, International Business Machines
# *   Corporation and others.  All Rights Reserved.
CURR_CLDR_VERSION = 21.0.1
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
#    |  CURR_SOURCE_LOCAL =   myLocale.txt ...
#
#  * To REPLACE the default list and only build with a few
#    locales:
#    _____________________________________________________
#    |  CURR_SOURCE = ar.txt ar_AE.txt en.txt de.txt zh.txt
#
#
# Generated by LDML2ICUConverter, from LDML source files.

# Aliases without a corresponding xx.xml file (see icu-config.xml & build.xml)
CURR_SYNTHETIC_ALIAS = az_AZ.txt az_Latn_AZ.txt en_RH.txt en_ZW.txt\
 fil_PH.txt ha_GH.txt ha_Latn_GH.txt ha_Latn_NE.txt ha_Latn_NG.txt\
 ha_NE.txt ha_NG.txt he_IL.txt id_ID.txt in.txt\
 in_ID.txt iw.txt iw_IL.txt ja_JP.txt ja_JP_TRADITIONAL.txt\
 kk_Cyrl_KZ.txt kk_KZ.txt mo.txt nb_NO.txt nn_NO.txt\
 no.txt no_NO.txt no_NO_NY.txt pa_Arab_PK.txt pa_Guru_IN.txt\
 pa_IN.txt pa_PK.txt ro_MD.txt sh.txt sh_BA.txt\
 sh_CS.txt sh_YU.txt shi_Latn_MA.txt shi_MA.txt sr_BA.txt\
 sr_CS.txt sr_Cyrl_CS.txt sr_Cyrl_RS.txt sr_Cyrl_YU.txt sr_Latn_BA.txt\
 sr_Latn_CS.txt sr_Latn_ME.txt sr_Latn_RS.txt sr_Latn_YU.txt sr_ME.txt\
 sr_RS.txt sr_YU.txt th_TH.txt th_TH_TRADITIONAL.txt tl.txt\
 tl_PH.txt tzm_Latn_MA.txt tzm_MA.txt uz_AF.txt uz_Arab_AF.txt\
 uz_Cyrl_UZ.txt uz_UZ.txt vai_LR.txt vai_Vaii_LR.txt zh_CN.txt\
 zh_HK.txt zh_Hans_CN.txt zh_Hant_MO.txt zh_Hant_TW.txt zh_MO.txt\
 zh_SG.txt zh_TW.txt


# All aliases (to not be included under 'installed'), but not including root.
CURR_ALIAS_SOURCE = $(CURR_SYNTHETIC_ALIAS)


# Ordinary resources
CURR_SOURCE = af.txt af_NA.txt agq.txt ak.txt\
 am.txt ar.txt as.txt asa.txt az.txt\
 az_Cyrl.txt az_Latn.txt bas.txt be.txt bem.txt\
 bez.txt bg.txt bm.txt bn.txt bo.txt\
 br.txt brx.txt bs.txt ca.txt cgg.txt\
 chr.txt cs.txt cy.txt da.txt dav.txt\
 de.txt de_LU.txt dje.txt dua.txt dyo.txt\
 ebu.txt ee.txt el.txt en.txt en_AU.txt\
 en_BB.txt en_BE.txt en_BM.txt en_BW.txt en_BZ.txt\
 en_CA.txt en_HK.txt en_JM.txt en_MT.txt en_NA.txt\
 en_NZ.txt en_PH.txt en_PK.txt en_SG.txt en_TT.txt\
 en_ZA.txt eo.txt es.txt es_AR.txt es_BO.txt\
 es_CL.txt es_CO.txt es_CR.txt es_DO.txt es_EC.txt\
 es_GT.txt es_HN.txt es_MX.txt es_NI.txt es_PA.txt\
 es_PE.txt es_PR.txt es_PY.txt es_US.txt es_UY.txt\
 es_VE.txt et.txt eu.txt ewo.txt fa.txt\
 fa_AF.txt ff.txt fi.txt fil.txt fo.txt\
 fr.txt fr_BI.txt fr_CA.txt fr_DJ.txt fr_GN.txt\
 fr_KM.txt fr_LU.txt ga.txt gl.txt gsw.txt\
 gu.txt guz.txt gv.txt ha.txt ha_Latn.txt\
 haw.txt he.txt hi.txt hr.txt hu.txt\
 hy.txt id.txt ig.txt ii.txt is.txt\
 it.txt ja.txt jmc.txt ka.txt kab.txt\
 kam.txt kde.txt kea.txt khq.txt ki.txt\
 kk.txt kk_Cyrl.txt kl.txt kln.txt km.txt\
 kn.txt ko.txt kok.txt ksb.txt ksf.txt\
 kw.txt lag.txt lg.txt ln.txt lt.txt\
 lu.txt luo.txt luy.txt lv.txt mas.txt\
 mas_TZ.txt mer.txt mfe.txt mg.txt mgh.txt\
 mk.txt ml.txt mr.txt ms.txt ms_BN.txt\
 mt.txt mua.txt my.txt naq.txt nb.txt\
 nd.txt ne.txt ne_IN.txt nl.txt nl_AW.txt\
 nl_CW.txt nl_SX.txt nmg.txt nn.txt nus.txt\
 nyn.txt om.txt om_KE.txt or.txt pa.txt\
 pa_Arab.txt pa_Guru.txt pl.txt ps.txt pt.txt\
 pt_AO.txt pt_MZ.txt pt_PT.txt pt_ST.txt rm.txt\
 rn.txt ro.txt rof.txt ru.txt rw.txt\
 rwk.txt saq.txt sbp.txt seh.txt ses.txt\
 sg.txt shi.txt shi_Latn.txt shi_Tfng.txt si.txt\
 sk.txt sl.txt sn.txt so.txt so_DJ.txt\
 so_ET.txt so_KE.txt sq.txt sr.txt sr_Cyrl.txt\
 sr_Cyrl_BA.txt sr_Latn.txt sv.txt sw.txt swc.txt\
 ta.txt ta_LK.txt te.txt teo.txt teo_KE.txt\
 th.txt ti.txt ti_ER.txt to.txt tr.txt\
 twq.txt tzm.txt tzm_Latn.txt uk.txt ur.txt\
 uz.txt uz_Arab.txt uz_Cyrl.txt uz_Latn.txt vai.txt\
 vai_Latn.txt vai_Vaii.txt vi.txt vun.txt xog.txt\
 yav.txt yo.txt zh.txt zh_Hans.txt zh_Hans_HK.txt\
 zh_Hans_MO.txt zh_Hans_SG.txt zh_Hant.txt zh_Hant_HK.txt zu.txt


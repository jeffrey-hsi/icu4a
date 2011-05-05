# *   Copyright (C) 1998-2011, International Business Machines
# *   Corporation and others.  All Rights Reserved.
ZONE_CLDR_VERSION = 2.0
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
#    |  ZONE_SOURCE_LOCAL =   myLocale.txt ...
#
#  * To REPLACE the default list and only build with a few
#    locales:
#    _____________________________________________________
#    |  ZONE_SOURCE = ar.txt ar_AE.txt en.txt de.txt zh.txt
#
#
# Generated by LDML2ICUConverter, from LDML source files.

# Aliases without a corresponding xx.xml file (see icu-config.xml & build.xml)
ZONE_SYNTHETIC_ALIAS = en_RH.txt in_ID.txt iw_IL.txt ja_JP_TRADITIONAL.txt\
 no_NO.txt no_NO_NY.txt th_TH_TRADITIONAL.txt


# All aliases (to not be included under 'installed'), but not including root.
ZONE_ALIAS_SOURCE = $(ZONE_SYNTHETIC_ALIAS) az_AZ.txt ha_GH.txt ha_NE.txt ha_NG.txt\
 in.txt iw.txt kk_KZ.txt mo.txt no.txt\
 pa_IN.txt pa_PK.txt sh.txt sh_BA.txt sh_CS.txt\
 sh_YU.txt shi_MA.txt sr_BA.txt sr_CS.txt sr_Cyrl_CS.txt\
 sr_Cyrl_YU.txt sr_Latn_CS.txt sr_Latn_YU.txt sr_ME.txt sr_RS.txt\
 sr_YU.txt tl.txt tl_PH.txt tzm_MA.txt uz_AF.txt\
 uz_UZ.txt vai_LR.txt zh_CN.txt zh_HK.txt zh_MO.txt\
 zh_SG.txt zh_TW.txt


# Ordinary resources
ZONE_SOURCE = af.txt agq.txt ak.txt am.txt\
 ar.txt as.txt asa.txt az.txt az_Cyrl.txt\
 az_Latn.txt az_Latn_AZ.txt bas.txt be.txt bem.txt\
 bez.txt bg.txt bm.txt bn.txt bo.txt\
 br.txt brx.txt bs.txt ca.txt cgg.txt\
 chr.txt cs.txt cy.txt da.txt dav.txt\
 de.txt dje.txt dua.txt dyo.txt ebu.txt\
 ee.txt el.txt en.txt en_AU.txt en_CA.txt\
 en_GB.txt en_NZ.txt en_ZA.txt en_ZW.txt eo.txt\
 es.txt es_419.txt et.txt eu.txt ewo.txt\
 fa.txt ff.txt fi.txt fil.txt fil_PH.txt\
 fo.txt fr.txt fr_CA.txt ga.txt gl.txt\
 gsw.txt gu.txt guz.txt gv.txt ha.txt\
 ha_Latn.txt ha_Latn_GH.txt ha_Latn_NE.txt ha_Latn_NG.txt haw.txt\
 he.txt he_IL.txt hi.txt hr.txt hu.txt\
 hy.txt id.txt id_ID.txt ig.txt ii.txt\
 is.txt it.txt ja.txt ja_JP.txt jmc.txt\
 ka.txt kab.txt kam.txt kde.txt kea.txt\
 khq.txt ki.txt kk.txt kk_Cyrl.txt kk_Cyrl_KZ.txt\
 kl.txt kln.txt km.txt kn.txt ko.txt\
 kok.txt ksb.txt ksf.txt kw.txt lag.txt\
 lg.txt ln.txt lt.txt lu.txt luo.txt\
 luy.txt lv.txt mas.txt mer.txt mfe.txt\
 mg.txt mgh.txt mk.txt ml.txt mr.txt\
 ms.txt mt.txt mua.txt my.txt naq.txt\
 nb.txt nb_NO.txt nd.txt ne.txt nl.txt\
 nmg.txt nn.txt nn_NO.txt nus.txt nyn.txt\
 om.txt or.txt pa.txt pa_Arab.txt pa_Arab_PK.txt\
 pa_Guru.txt pa_Guru_IN.txt pl.txt ps.txt pt.txt\
 pt_PT.txt rm.txt rn.txt ro.txt ro_MD.txt\
 rof.txt ru.txt rw.txt rwk.txt saq.txt\
 sbp.txt seh.txt ses.txt sg.txt shi.txt\
 shi_Latn.txt shi_Latn_MA.txt shi_Tfng.txt si.txt sk.txt\
 sl.txt sn.txt so.txt sq.txt sr.txt\
 sr_Cyrl.txt sr_Cyrl_BA.txt sr_Cyrl_RS.txt sr_Latn.txt sr_Latn_BA.txt\
 sr_Latn_ME.txt sr_Latn_RS.txt sv.txt sw.txt swc.txt\
 ta.txt te.txt teo.txt th.txt th_TH.txt\
 ti.txt to.txt tr.txt twq.txt tzm.txt\
 tzm_Latn.txt tzm_Latn_MA.txt uk.txt ur.txt uz.txt\
 uz_Arab.txt uz_Arab_AF.txt uz_Cyrl.txt uz_Cyrl_UZ.txt uz_Latn.txt\
 vai.txt vai_Latn.txt vai_Vaii.txt vai_Vaii_LR.txt vi.txt\
 vun.txt xog.txt yav.txt yo.txt zh.txt\
 zh_Hans.txt zh_Hans_CN.txt zh_Hans_HK.txt zh_Hans_MO.txt zh_Hans_SG.txt\
 zh_Hant.txt zh_Hant_HK.txt zh_Hant_MO.txt zh_Hant_TW.txt zu.txt


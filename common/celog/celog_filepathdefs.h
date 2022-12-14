/*************************************************************************
 *
 * Compliant Enterprise Logging
 *
 * Declarations for integers representing source file paths
 *
 ************************************************************************/

#ifndef __CELOG_FILEPATHDEFS_H__
#define __CELOG_FILEPATHDEFS_H__



typedef enum
{
  /* CELog3 */
  CELOG_FILEPATH_CELOG3_MIN = 0,
  CELOG_FILEPATH_PROD_COMMON_CELOG3_TEST_CELOG_TEST_CPP,
  
  /* BasePEP */
  CELOG_FILEPATH_BASEPEP_MIN = 100,
  CELOG_FILEPATH_PROD_PEP_ENDPOINT_WDE_BASEPEP_INCLUDE_NLCDBURN_H,
  CELOG_FILEPATH_PROD_PEP_ENDPOINT_WDE_BASEPEP_SRC_DROPTARGETPROXY_CPP,
  CELOG_FILEPATH_PROD_PEP_ENDPOINT_WDE_BASEPEP_SRC_ENCPROC_CPP,
  CELOG_FILEPATH_PROD_PEP_ENDPOINT_WDE_BASEPEP_SRC_HOOKFUN_CPP,
  CELOG_FILEPATH_PROD_PEP_ENDPOINT_WDE_BASEPEP_SRC_HOOKTOOL_CPP,
  CELOG_FILEPATH_PROD_PEP_ENDPOINT_WDE_BASEPEP_SRC_INJECTOR_CPP,
  CELOG_FILEPATH_PROD_PEP_ENDPOINT_WDE_BASEPEP_SRC_MAILAUTOWRAP_CPP,
  CELOG_FILEPATH_PROD_PEP_ENDPOINT_WDE_BASEPEP_SRC_MODULESCOPE_CPP,
  CELOG_FILEPATH_PROD_PEP_ENDPOINT_WDE_BASEPEP_SRC_MSOFFICECONTEXT_CPP,
  CELOG_FILEPATH_PROD_PEP_ENDPOINT_WDE_BASEPEP_SRC_MSWORDCONTEXT_CPP,
  CELOG_FILEPATH_PROD_PEP_ENDPOINT_WDE_BASEPEP_SRC_NLFILEOPERATION_CPP,
  CELOG_FILEPATH_PROD_PEP_ENDPOINT_WDE_BASEPEP_SRC_NLPRINTPHOTOS_CPP,
  CELOG_FILEPATH_PROD_PEP_ENDPOINT_WDE_BASEPEP_SRC_PDFMANAGER_CPP,
  CELOG_FILEPATH_PROD_PEP_ENDPOINT_WDE_BASEPEP_SRC_WDECRYPTO_CPP,
  CELOG_FILEPATH_PROD_PEP_ENDPOINT_WDE_BASEPEP_SRC_WDEINTERNET_CPP,
  CELOG_FILEPATH_PROD_PEP_ENDPOINT_WDE_BASEPEP_SRC_WDEOLEDATATRANSFER_CPP,
  CELOG_FILEPATH_PROD_PEP_ENDPOINT_WDE_BASEPEP_SRC_WDEPRINT_CPP,
  CELOG_FILEPATH_PROD_PEP_ENDPOINT_WDE_BASEPEP_SRC_WDESDKSUPPORT_CPP,
  CELOG_FILEPATH_PROD_PEP_ENDPOINT_WDE_BASEPEP_SRC_WDEVISUALLABELING_CPP,

  /* iePEP */
  CELOG_FILEPATH_IEPEP_MIN = 200,
  CELOG_FILEPATH_PROD_PEP_ENDPOINT_WDE_IEPEP_SRC_IEPEP_CPP,
  CELOG_FILEPATH_PROD_PEP_ENDPOINT_WDE_IEPEP_SRC_IEOHJ_CPP,
  CELOG_FILEPATH_PROD_PEP_ENDPOINT_WDE_IEPEP_SRC_HOOK_CPP,
  CELOG_FILEPATH_PROD_PEP_ENDPOINT_WDE_IEPEP_SRC_EVENT_HANDLER_CPP,
  CELOG_FILEPATH_PROD_PEP_ENDPOINT_WDE_IEPEP_SRC_EVALUATE_CPP,
  CELOG_FILEPATH_PROD_PEP_ENDPOINT_WDE_IEPEP_SRC_NL_BASE64_CPP,
  CELOG_FILEPATH_PROD_PEP_ENDPOINT_WDE_IEPEP_SRC_NL_ActionHandler_CPP,

  /* Office PEP */
  CELOG_FILEPATH_OFFICEPEP_MIN = 300,

  /* Adobe PEP */
  CELOG_FILEPATH_ADOBEPEP_MIN = 400,
  CELOG_FILEPATH_PROD_PEP_ENDPOINT_ADOBEPEP_FORCEADOBEPEP_SRC_APIHOOK_CPP,
  CELOG_FILEPATH_PROD_PEP_ENDPOINT_ADOBEPEP_FORCEADOBEPEP_SRC_DLLMAIN_CPP,
  CELOG_FILEPATH_PROD_PEP_ENDPOINT_ADOBEPEP_FORCEADOBEPEP_SRC_ENCRYPT_CPP,
  CELOG_FILEPATH_PROD_PEP_ENDPOINT_ADOBEPEP_FORCEADOBEPEP_SRC_OPOVERLAYINFO_CPP,
  CELOG_FILEPATH_PROD_PEP_ENDPOINT_ADOBEPEP_FORCEADOBEPEP_SRC_OVERLAY_CPP,
  CELOG_FILEPATH_PROD_PEP_ENDPOINT_ADOBEPEP_PLUGIN_SAMPLESADOBEPEP_INCLUDE_OBMGR_H,
  CELOG_FILEPATH_PROD_PEP_ENDPOINT_ADOBEPEP_PLUGIN_SAMPLESADOBEPEP_SRC_STARTERINIT_CPP,
  CELOG_FILEPATH_PROD_PEP_ENDPOINT_ADOBEPEP_PLUGIN_SAMPLESADOBEPEP_INCLUDE_WINAPIFILE_H,
  CELOG_FILEPATH_PROD_PEP_ENDPOINT_ADOBEPEP_PLUGIN_SAMPLESADOBEPEP_INCLUDE_NLTAG_H,
  CELOG_FILEPATH_PROD_PEP_ENDPOINT_ADOBEPEP_PLUGIN_SAMPLESADOBEPEP_INCLUDE_POLICY_H,
  CELOG_FILEPATH_PROD_PEP_ENDPOINT_ADOBEPEP_PLUGIN_SAMPLESADOBEPEP_INCLUDE_MENUITEM_H,
  CELOG_FILEPATH_PROD_PEP_ENDPOINT_ADOBEPEP_PLUGIN_SAMPLESADOBEPEP_INCLUDE_SAVEASOBLIGATION_H,
  CELOG_FILEPATH_PROD_PEP_ENDPOINT_ADOBEPEP_PLUGIN_SAMPLESADOBEPEP_INCLUDE_DOENCRYPTOBLIGATION_H,
  CELOG_FILEPATH_PROD_PEP_ENDPOINT_ADOBEPEP_PLUGIN_SAMPLESADOBEPEP_INCLUDE_PDDOCLNSERTPAGES_H,
  CELOG_FILEPATH_PROD_PEP_ENDPOINT_ADOBEPEP_PLUGIN_SAMPLESADOBEPEP_SRC_UTILITIES_CPP,
  CELOG_FILEPATH_PROD_PEP_ENDPOINT_ADOBEPEP_PLUGIN_SAMPLESADOBEPEP_SRC_ENCRYPT_CPP,
  CELOG_FILEPATH_PROD_PEP_ENDPOINT_ADOBEPEP_PLUGIN_SAMPLESADOBEPEP_SRC_CLASSIFY_CPP,
  CELOG_FILEPATH_PROD_PEP_ENDPOINT_ADOBEPEP_PLUGIN_SAMPLESADOBEPEP_SRC_SEND_CPP,
  CELOG_FILEPATH_PROD_PEP_ENDPOINT_ADOBEPEP_PLUGIN_SAMPLESADOBEPEP_SRC_OVERLAY_CPP,
  CELOG_FILEPATH_PROD_PEP_ENDPOINT_ADOBEPEP_PLUGIN_SAMPLES_ADOBEPEP_SRC_PDDOCINSERTPAGES_CPP,
  
  /* RMC */
  CELOG_FILEPATH_RMC_MIN = 500,
  CELOG_FILEPATH_PROD_PEP_ENDPOINT_SE_NL_SYSENCRYPTION_USER_NLSELIB_CPP,
  CELOG_FILEPATH_PROD_PEP_ENDPOINT_SE_NL_SYSENCRYPTION_USER_NLSEPLUGIN_CPP,
  CELOG_FILEPATH_PROD_PEP_ENDPOINT_SE_NL_SYSENCRYPTION_USER_NLSESDKWRAPPER_CPP,
  CELOG_FILEPATH_PROD_PEP_ENDPOINT_SE_NL_SYSENCRYPTIONFW_USER_NLSELIB_CPP,
  CELOG_FILEPATH_PROD_PEP_ENDPOINT_SE_NL_SYSENCRYPTIONFW_USER_NLSEPLUGIN_CPP,
  CELOG_FILEPATH_PROD_PEP_ENDPOINT_SE_NL_SYSENCRYPTIONFW_USER_NLSESDKWRAPPER_CPP,
  CELOG_FILEPATH_PROD_PEP_ENDPOINT_SE_NL_SYSENC_LIB_SRC_ALTDATASTREAM_CPP,
  CELOG_FILEPATH_PROD_PEP_ENDPOINT_SE_NL_SYSENC_LIB_SRC_NLSELIB_CPP,
  CELOG_FILEPATH_PROD_PEP_ENDPOINT_SE_NL_SYSENC_LIB_SRC_NLSELIBFW_CPP,
  CELOG_FILEPATH_PROD_PEP_ENDPOINT_SE_NL_SYSENC_LIB_SRC_NLSESDKWRAPPER_CPP,

  /* ResAttr */
  CELOG_FILEPATH_RESATTTR_MIN = 600,

  /* Tagging Libs */
  CELOG_FILEPATH_TAGGINGLIB_MIN = 700,

  /* PAF */
  CELOG_FILEPATH_PAF_MIN = 800,

  /* NE */
  CELOG_FILEPATH_NE_MIN = 900,

  /* OE */
  CELOG_FILEPATH_OE_MIN = 1000,

  /* RDE */
  CELOG_FILEPATH_RDE_MIN = 1100,
  CELOG_FILEPATH_PROD_PEP_ENDPOINT_RDE_NLPNPDRIVERINSTALLER_SRC_NLPNPDRIVERINSTALLER_CPP
} celog_filepathint_t;



#endif /* __CELOG_FILEPATHDEFS_H__ */

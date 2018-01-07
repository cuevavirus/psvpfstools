#include "PfsKeyGenerator.h"

#include <string>
#include <cstring>
#include <stdexcept>

#include "PfsKeys.h"
#include "IcvPrimitives.h"
#include "PfsCryptEngine.h"
#include "SecretGenerator.h"

//[TESTED]
int generate_enckeys(unsigned char* dec_key, unsigned char* tweak_enc_key, const unsigned char* klicensee, std::uint32_t icv_salt)
{
   int saltin[2] = {0};
   unsigned char base0[0x14] = {0};
   unsigned char base1[0x14] = {0};
   
   unsigned char drvkey[0x14] = {0};

   icv_set_sw(base0, klicensee, 0x10); //calculate hash of klicensee

   saltin[0] = icv_salt;

   // derive key 0

   saltin[1] = 1;
   
   icv_set_sw(base1, (unsigned char *)saltin, 8); //calculate hash of salt 0

   icv_contract(drvkey, base0, base1); //calculate hash from combination of salt 0 hash and klicensee hash

   memcpy(dec_key, drvkey, 0x10);  //copy derived key

   // derive key 1
   
   saltin[1] = 2;

   icv_set_sw(base1, (unsigned char*)saltin, 8); //calculate hash of salt 1

   icv_contract(drvkey, base0, base1); //calculate hash from combination of salt 1 hash and klicensee hash

   memcpy(tweak_enc_key, drvkey, 0x10); //copy derived key

   return 0;
}

//[TESTED]
int gen_iv(unsigned char* tweak_enc_key, std::uint32_t files_salt, std::uint32_t icv_salt)
{
   unsigned char drvkey[0x14] = {0};

   if(files_salt == 0)
   {
      int saltin0[1] = {0};
      saltin0[0] = icv_salt;

      icv_set_hmac_sw(drvkey, hmac_key0, (unsigned char*)saltin0, 4); // derive key with one salt
   }
   else
   {
      int saltin1[2] = {0};
      saltin1[0] = files_salt;
      saltin1[1] = icv_salt;
      
      icv_set_hmac_sw(drvkey, hmac_key0, (unsigned char*)saltin1, 8); // derive key with two salts
   }

   memcpy(tweak_enc_key, drvkey, 0x10); //copy derived key

   return 0;
}

//---------------------

//[TESTED]
int scePfsUtilGetSDKeys(unsigned char* dec_key, unsigned char* tweak_enc_key, const unsigned char* klicensee, std::uint32_t files_salt, std::uint32_t icv_salt)
{
  //files_salt is ignored
  return generate_enckeys(dec_key, tweak_enc_key, klicensee, icv_salt);
}

//[TESTED]
int scePfsUtilGetGDKeys(unsigned char* dec_key, unsigned char* tweak_enc_key, const unsigned char* klicensee, std::uint32_t files_salt, std::uint16_t flag, std::uint32_t icv_salt)
{
   if((flag & 2) > 0)
   {
      memcpy(dec_key, klicensee, 0x10);

      return gen_iv(tweak_enc_key, files_salt, icv_salt);
   }
   else
   {
      return generate_enckeys(dec_key, tweak_enc_key, klicensee, icv_salt);
   }
}

//[TESTED]
int scePfsUtilGetGDKeys2(unsigned char* dec_key, unsigned char* tweak_enc_key, const unsigned char* klicensee, std::uint16_t ignored_flag, std::uint16_t ignored_key_id, const unsigned char* dbseed, std::uint32_t dbseed_len)
{
   unsigned char drvkey[0x14] = {0};

   icv_set_hmac_sw(drvkey, hmac_key0, dbseed, dbseed_len);

   memcpy(dec_key, klicensee, 0x10);

   memcpy(tweak_enc_key, drvkey, 0x10);

   return 0;
}

//---------------------

//0xD appeared on 3.60

bool is_gamedata(std::uint16_t flag)
{
   int index = flag & 0xFFFF;
   
   if(index > 0x21)
      return false;
   
   switch(index)
   {
      case 0x02:
      case 0x03:
      case 0x0A:
      case 0x0B:
      case 0x0D:
      case 0x20:
      case 0x21:
         return true;

      default:
         return false;
   }
}

const unsigned char* isec_dbseed(const derive_keys_ctx* drv_ctx)
{
   //unk_40 must be equal to 0 or 3 AND 
   //version should be > 1 showing that ricv seed is supported

   if((drv_ctx->unk_40 != 0 && drv_ctx->unk_40 != 3) || drv_ctx->icv_version <= 1)
      return 0;
   else
      return drv_ctx->dbseed;
}

//---------------------

struct filesdb_t
{
   std::uint16_t pmi_bcl_flag;
   std::uint16_t mode_index;
};

struct pfsfile_t
{
   std::uint16_t flag0;
};

//---------------------

struct pfs_mode_settings
{
   std::uint32_t unk_0;
   std::uint32_t unk_4;
   std::uint32_t unk_8;
   std::uint32_t unk_C;

   std::uint32_t unk_10;
   std::uint32_t unk_14;
   std::uint32_t unk_18;
   std::uint32_t unk_1C;

   std::uint32_t unk_20;
   std::uint32_t unk_24;
   std::uint32_t unk_28;
   std::uint32_t unk_2C;

   std::uint32_t unk_30;
   std::uint32_t unk_34;
   std::uint32_t unk_38;
   std::uint32_t unk_3C;
};

//00 - fake
pfs_mode_settings gFakeSetting =       { 0x00000002, 0x00000002, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 
                                         0x90909090, 0x90909090, 0x90909090, 0x90909090, 0x90909090, 0x90909090};

//02 - GD (Game Data)
pfs_mode_settings gGdgpSetting =       { 0x00000001, 0x00000000, 0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000001, 0x00000001, 0x00000001, 
                                         0x90909090, 0x90909090, 0x90909090, 0x90909090, 0x90909090, 0x90909090};

//03 - GD (?)
pfs_mode_settings gGpwrSetting =       { 0x00000001, 0x00000000, 0x00000001, 0x00000000, 0x00000000, 0x00000180, 0x000001C0, 0x00000001, 0x00000000, 0x00000001, 
                                         0x90909090, 0x90909090, 0x90909090, 0x90909090, 0x90909090, 0x90909090};

//04, 08 - AC (AC Pseudo Drive)
pfs_mode_settings gAcSetting =         { 0x00000000, 0x00000001, 0x00000001, 0x00000102, 0x00000102, 0x00000180, 0x000001C0, 0x00000000, 0x00000000, 0x00000000, 
                                         0x90909090, 0x90909090, 0x90909090, 0x90909090, 0x90909090, 0x90909090};

//05, 06, 07, 09 (Save Data)
pfs_mode_settings gSdSetting =         { 0x00000000, 0x00000001, 0x00000001, 0x00000102, 0x00000102, 0x00000180, 0x000001C0, 0x00000000, 0x00000000, 0x00000000, 
                                         0x90909090, 0x90909090, 0x90909090, 0x90909090, 0x90909090, 0x90909090};

//0A, 0B (in terms of pfs - pack means unicv.db - pack of icv files)
pfs_mode_settings gPackSetting =       { 0x00000001, 0x00000000, 0x00000001, 0x00000102, 0x00000102, 0x00000180, 0x000001C0, 0x00000000, 0x00000000, 0x00000001, 
                                         0x90909090, 0x90909090, 0x90909090, 0x90909090, 0x90909090, 0x90909090};

//0C - AC (AC Pseudo Drive)
pfs_mode_settings gAcroSetting =       { 0x00000000, 0x00000001, 0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000001, 0x00000001, 0x00000000, 
                                         0x90909090, 0x90909090, 0x90909090, 0x90909090, 0x90909090, 0x90909090};

//14 - REDIRECT (Redirect Pseudo Drive)
pfs_mode_settings gRedirectRoSetting = { 0x00000002, 0x00000002, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000001, 0x00000001, 0x00000000, 
                                         0x90909090, 0x90909090, 0x90909090, 0x90909090, 0x90909090, 0x90909090};

//15, 16, 17 - REDIRECT (Redirect Pseudo Drive)
pfs_mode_settings gRedirectSetting =   { 0x00000002, 0x00000002, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 
                                         0x90909090, 0x90909090, 0x90909090, 0x90909090, 0x90909090, 0x90909090};

//20, 21 - AC (AC Pseudo Drive)
pfs_mode_settings gAcContSetting =     { 0x00000001, 0x00000000, 0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000001, 0x00000001, 0x00000001, 
                                         0x90909090, 0x90909090, 0x90909090, 0x90909090, 0x90909090, 0x90909090};

//settings correlate with is_gamedata
//only 0x02, 0x03, 0x0A, 0x0B, 0x0D, 0x20, 0x21 - have unk_4 == 0 (except from 0xD which does not correlate with 3.60)
//meaning that probably 0 means that this is gamedata ?
//names of settings also kinda correlate (GD, SD, AC, REDIRECT etc)

//looking at other parts of the code - i see that if unk_4 > 0 - this is related to icv (not to unicv)

pfs_mode_settings* scePfsGetModeSetting(std::uint16_t mode_index)
{
   int index = mode_index & 0xFFFF;
   
   if(index > 0x21)
      throw std::runtime_error("Invalid index");
   
   switch(index)
   {
      case 0x00: 
         return &gFakeSetting;       // unk_4 = 0x00000002

      case 0x02: 
         return &gGdgpSetting;       // unk_4 = 0x00000000 - GAME

      case 0x03: 
         return &gGpwrSetting;       // unk_4 = 0x00000000 - GAME

      case 0x04:
      case 0x08: 
         return &gAcSetting;         // unk_4 = 0x00000001

      case 0x05:
      case 0x06:
      case 0x07:
      case 0x09:
         return &gSdSetting;         // unk_4 = 0x00000001

      case 0x0A:
      case 0x0B:
         return &gPackSetting;       // unk_4 = 0x00000000 - GAME

      case 0x0C: 
         return &gAcroSetting;       // unk_4 = 0x00000001

      case 0x14: 
         return &gRedirectRoSetting; // unk_4 = 0x00000002

      case 0x15:
      case 0x16:
      case 0x17:
         return &gRedirectSetting;   // unk_4 = 0x00000002

      case 0x20: 
      case 0x21: 
         return &gAcContSetting;     // unk_4 = 0x00000000 - GAME

      default:
         throw std::runtime_error("Invalid index");
   }
}

//--------------------
//pseudo impl of start and restart that shows relation between translated setting and db type
//this allows us to derive the map:

//0 - SCEIFTBL
//1 - SCEICVDB
//2 - SCEINULL
//3 - SCEIFTBL

//which correlates exactly to isec->unk40 and isec_dbseed
//only mode 0 and 3 allows to select seed (icv does not support seeds)

void isec_restart(unsigned int some_pfs_setting)
{
  if (some_pfs_setting == 0)
  {
    //isec_restart_ro(isec, &files->ro, fa->nid, secret, size);// SCEIFTBL
  }
  else if (some_pfs_setting == 1)
  {
    //rw_icv_db_version = scePfsRwicvDbVersion(files->version_index);
    //isec_restart_rw(isec, &files->lm2, fa, secret, size, rw_icv_db_version);// SCEICVDB
  }
  else if (some_pfs_setting == 2)
  {
    //null_icv_db_version = scePfsNullicvDbVersion(files->version_index);
    //isec_restart_null(isec, fa, secret, size, null_icv_db_version);// SCEINULL
  }
  else if (some_pfs_setting == 3)
  {
    //isec_restart_nullro(isec, &files->ro, fa->nid, size);// SCEIFTBL
  }
  else
  {
    //0x80142009;
  }
}

void isec_start(unsigned int some_pfs_setting)
{
  if (some_pfs_setting == 0)
  {
    //isec_start_ro(isec, arg4 + 112, *(_DWORD *)(a3 + 20), a2, Src);// SCEIFTBL
  }
  else if (some_pfs_setting == 1)
  {
    //rw_icv_db_version = scePfsRwicvDbVersion(*(_DWORD *)(arg4 + 2244));
    //isec_start_rw(isec, arg4 + 56, a3, a2, Src, rw_icv_db_version);// SCEICVDB
  }
  else if (some_pfs_setting == 2)
  {
    //null_icv_db_version = scePfsNullicvDbVersion(*(_DWORD *)(arg4 + 2244));
    //isec_start_null(isec, a3, a2, Src, null_icv_db_version);// SCEINULL
  }
  else if (some_pfs_setting == 3)
  {
    //isec_start_nullro(isec, arg4 + 112, *(_DWORD *)(a3 + 20), a2);// SCEIFTBL
  }
  else
  {
    //0x80142009;
  }
}

//--------------------

//there is another function that uses mode_index as scePfsGetModeSetting
//it is scePfsGetImageSpec
//here is example in pfspack_init4:
//v18 = scePfsGetModeSetting(a5);
//v17 = scePfsGetImageSpec(a5);
//can then be used to check type of image:
//scePfsIsRoImage(v17)

//this is particulary interesting piece of code in _main function

/*
char v44[4];

*(_DWORD *)v44 = 0;

v39 = scePfsGetImageSpec(HIWORD(v40));
if ( !*(_DWORD *)v44 )
{
   if ( scePfsIsRoImage(v39) )
   *(_DWORD *)v44 = scePfsRodbVersionUpper();
   else
   *(_DWORD *)v44 = scePfsRwdbVersionUpper();
}
*/

//this shows that we can distinguish between ro and rw db by checking image spec

//does this correlate with scePfsGetModeSetting? Yes 
//only these indexes should correspond to game data : 0x02, 0x03, 0x0A, 0x0B, 0x0D, 0x20, 0x21
//if we compare switch with scePfsIsRoImage - these indexes map exactly to 1 or 4 which is RO data (game data)

//0xD does not correlate with 3.60

bool scePfsIsRoImage(std::uint16_t image_spec)
{
  return image_spec == 1 || image_spec == 4;
}

std::uint16_t scePfsGetImageSpec(std::uint16_t mode_index)
{
   int index = mode_index & 0xFFFF;

   if(index > 0x21)
      return 0xFFFF;
   
   switch(index)
   {
      case 0x00: 
      case 0x14: 
      case 0x15: 
      case 0x16: 
      case 0x17: 
         return 0 & 0xFFFF;

      case 0x02: 
      case 0x03: 
      case 0x0A: 
         return 1 & 0xFFFF; // IsRoImage - GAME

      case 0x04: 
      case 0x08: 
      case 0x0C: 
         return 3 & 0xFFFF;

      case 0x05:
      case 0x06:
      case 0x07:
      case 0x09: 
         return 2 & 0xFFFF;
      
      case 0x0B:
      case 0x20:
      case 0x21: 
         return 4 & 0xFFFF; // IsRoImage - GAME

      default: 
         return 0xFFFF;
   }
}

int scePfsCheckImage(std::uint16_t mode_index, std::uint16_t image_spec)
{
  if(scePfsGetImageSpec(mode_index) != image_spec)
     return -1;
  return 0;
}

//---------------------
//scePfsGetModeSetting selects setting instance from global array of settings using filesdb_t* fl->mode_index
//we can then select settings->unk_4
//settings->unk_4 is transformed to some_pfs_setting
//using translate_setting_start or translate_setting_restart with flag0 which comes from pfsf->flag0
//some_pfs_setting is assigned to isec
//isec->unk40 = some_pfs_setting;
//in
//int __cdecl isec_start(isec_t *isec, int arg4, int a3, int a2, void *Src, unsigned int some_pfs_setting)
//int __cdecl isec_restart(isec_t *isec, filesdb_t *files, file_acces_t *fa, char *secret, __int64 size, unsigned int some_pfs_setting)
//isec->unk40 is later used in isec_dbseed
//to select seed (v2 = isec->unk40)
//looks like isec_t is the same type as derive_keys_ctx (type in my code)

//---------------------
//flag 0 comes to CryptEngineData from pfsfile_t *pfsf

//unsigned int __cdecl pfsfile_pwrite(pfsfile_t *pfsf, int pad_size_total, int a3, __int64 size)
//int __cdecl secure_drive_pwrite(file_acces_t *fa, int unicv_page_salt, __int64 size0, __int16 flag0, filesdb_t *fl, isec_t *isec, int pad_size_total, int arg20, __int64 size, _DWORD *arg2C)
//signed int __cdecl _secure_drive_pwrite(file_acces_t *fa, int unicv_page_salt, __int64 size0, __int16 flag0, filesdb_t *fl, isec_t *isec, int pad_size_total, int pad_size, __int64 size, int *pad_size_res)
//int __cdecl setup_crypt_packet_keys(CryptEngineData *ctx, int unicv_page_salt, __int64 size, __int16 flag0, int mode_setting_unk0, isec_t *isec_ctx, filesdb_t *fl)
//CryptEngineData *ctx
//ctx->flag0 = flag0;

//---------------------

//in is_gamedata and in key derrivation - pmi_bcl_flag that is used comes from

//CryptEngineData *ctx
//ctx->pmi_bcl_flag
//this flag is set in setup_crypt_packet_keys
//ctx->pmi_bcl_flag = fl->pmi_bcl_flag;
//where filesdb_t *fl

//---------------------

//what we still need to figure out 3 variables:
//how filesdb_t* fl->pmi_bcl_flag is initialized - this affects CryptEngineData * ctx->pmi_bcl_flag
//how filesdb_t* fl->mode_index is initialized - this affects ctx->mode_index and isect_t* isec->unk40
//how pfsfile_t* pfsf->flag0 is initialized - this affects CryptEngineData* ctx->flag0
//how pfsfile_t* pfsf->flag0 is initialized - this affects isect_t* isec->unk40

//---------------------

//mode_index and pmi_bcl_flag are set in pfspack_init4
//it is called either from main
//or from pfspack_init3
//   from pfspack_init2 - which is not referenced

//flag0 is initialized in:
//pfsfile_open 
//pfsfile_mkdir

//also check get_file_mode

//looks like with scePfsACSetFSAttrByMode

//---------------------

//here is derivation of mode_index and pmi_bcl_flag from _main function
//everything starts with -s option
//that goes to base_flags
// 0:gamedata
// 1:savedata
// 2:AC ROOT 
// 3:ACID DIR

//mode_index_pmi_bcl_flag then goes to pfspack_init4

enum pfs_image_types : std::uint16_t
{
   gamedata = 0,
   savedata = 1,
   ac_root = 2,
   acid_dir = 3
};

int main_flags(pfs_image_types img_type, std::uint16_t* mode_index, std::uint16_t* pmi_bcl_flag)
{
   switch(img_type)
   {
   case gamedata:
      {
         *mode_index = 0x0A; // gPackSetting - ro image
         *pmi_bcl_flag = 1;
         *pmi_bcl_flag |= 2;
      }
      break;
   case savedata:
      {
         *mode_index = 0x05; // gSdSetting - rw image
         *pmi_bcl_flag = 1;
      }
      break;
   case ac_root:
      {
         *mode_index = 0x04; // gAcSetting - rw image
         *pmi_bcl_flag = 1;
      }
      break;
   case acid_dir:
      {
         *mode_index = 0x0B; // gPackSetting - ro image
         *pmi_bcl_flag = 1;
         *pmi_bcl_flag |= 2;
      }
      break;
   default:
      throw std::runtime_error("Invalid index");
   }

   return 0;
}


//---------------------

//flag map - derrivation up to this point

//setup_icvdb
//isec_start

//this sets derive_keys_ctx.unk_40

//flag0 comes from pfsf->flag0
//v13 = setup_icvdb(pfsf->files, pfsf->unicv_page_salt, pfsf->flag0, pfsf->unk24, (int)&v9);
std::uint32_t translate_setting_start(pfsfile_t* pfsf, filesdb_t* fl)
{
  pfs_mode_settings* settings = scePfsGetModeSetting(fl->mode_index);

  std::uint32_t some_pfs_setting = settings->unk_4;
 
  if(settings->unk_4 == 1 && (pfsf->flag0 & 0x2000 || pfsf->flag0 & 0x8000))
    some_pfs_setting = 2;

  return some_pfs_setting;
}

//init_icvobj
//isec_restart

//this sets derive_keys_ctx.unk_40

//flag0 comes from pfsf->flag0
//v13 = init_icvobj(&pfsf->isec, pfsf->files, &pfsf->files->fa2, pfsf->unicv_page_salt, pfsf->flag0, pfsf->size);
std::uint32_t translate_setting_restart(pfsfile_t* pfsf, filesdb_t* fl)
{
   pfs_mode_settings* settings = scePfsGetModeSetting(fl->mode_index);

   std::uint32_t some_pfs_setting = settings->unk_4;

   if(settings->unk_4 == 1 && (pfsf->flag0 & 0x2000 || pfsf->flag0 & 0x8000))
      some_pfs_setting = 2;

   if(settings->unk_4 == 0 && pfsf->flag0 & 0x400 )
      some_pfs_setting = 3;
  
  return some_pfs_setting;
}

//this sets dctx->unk_40 field that can be used in isec_dbseed
void set_drv_ctx(derive_keys_ctx* dctx, pfsfile_t* pfsf, filesdb_t* fl)
{
   dctx->unk_40 = translate_setting_restart(pfsf, fl);
}

//---------------------

int setup_crypt_packet_keys(CryptEngineData* data, const derive_keys_ctx* drv_ctx)
{
   if(is_gamedata(data->pmi_bcl_flag))
   {
      if(isec_dbseed(drv_ctx))
      {  
         scePfsUtilGetGDKeys2(data->dec_key, data->tweak_enc_key, data->klicensee, data->pmi_bcl_flag, data->key_id, isec_dbseed(drv_ctx), 0x14);  
      }
      else
      {
         scePfsUtilGetGDKeys(data->dec_key, data->tweak_enc_key, data->klicensee, data->files_salt, data->pmi_bcl_flag, data->icv_salt);
      }
   }
   else
   {
      scePfsUtilGetSDKeys(data->dec_key, data->tweak_enc_key, data->klicensee, data->files_salt, data->icv_salt);
   }

   return scePfsUtilGetSecret(data->secret, data->klicensee, data->files_salt, data->pmi_bcl_flag, data->icv_salt, data->key_id);
}
#include "audiocontrol.h"
#include <QDateTime>
#include "bdlog.h"
static const char* streamInfo[12][2] = {
	{"default","softmaster"},//LocalMusic
	{"plug:softvol2","softmaster2"},//BtMusic
	{"plug:softvol1","softmaster1"},//Navi
	{"plug:softvol3","softmaster3"},//Calling
	{"plug:softvol4","softmaster4"},//tts
	{"plug:softvol5","softmaster5"},//alarm
	{"plug:softvol6","softmaster6"},//keybeep
	{"plug:softvol8","softmaster8"},//ring
	{"plug:softvol13","softmaster13"},//hicar_alarm
	{"plug:softvol14","softmaster14"},//hicar_notice
	{"plug:softvol11","softmaster11"},//volumeSlide
	{"plug:softvol12","softmaster12"},//carlock
};

AudioControl::AudioControl()
{
	snd_pcm_t *handle = nullptr;
	int ret  = 0, index = 0;
	for( index = 0; index <= AUDIO_LOCAL_CARLOCK; ++index){
		
		ret = snd_pcm_open(&handle, streamInfo[index][0], SND_PCM_STREAM_PLAYBACK, 0);
		if(ret){
			VLOGE("@audio-error AudioControl init snd_pcm(%d) error",index);
			return;
		}	
		snd_pcm_close(handle);
		handle = nullptr;

		if(index == AUDIO_LOCAL_CARLOCK){
			VLOGD("@audio-ac %s %s",__func__, QDateTime::currentDateTime().toString("hh:mm:ss.zzz").toUtf8().constData());
		}
	}
}


static snd_mixer_t * gMixer = NULL;
static snd_mixer_elem_t * gDuckElement = NULL;

#define  require_noerr( err, exit ); \
    do { \
    if (err) { \
    goto exit;\
    } \
    } while (0)

#if( !defined( Max ) )
#define	Max( X, Y )		( ( (X) > (Y) ) ? (X) : (Y) )
#endif

#if( !defined( Min ) )
#define	Min( X, Y )		( ( (X) < (Y) ) ? (X) : (Y) )
#endif

#define Clamp( x, a, b )		Max( (a), Min( (b), (x) ) )

int AudioControl::ALSAMixerOpen(int stream_name, int ctrlname)
{
    snd_mixer_selem_id_t *sid;
    snd_pcm_t 			*handle;
    snd_mixer_t *lMixer = NULL;
    snd_mixer_elem_t *lDuckElement = NULL;
    int err;
    //int retry_cnt = 0;
    //const char* stream_audioName = NULL;
    //const char* stream_ctrlName = NULL;
    qDebug()<<"stream_name:"<<stream_name<<"ctrlname:"<<ctrlname;

    if (gMixer && gDuckElement)
        return 0;

    err = snd_pcm_open(&handle, streamInfo[stream_name][0], SND_PCM_STREAM_PLAYBACK, 0);
    require_noerr(err, exit);
    snd_pcm_close(handle);
    handle = NULL;

    err = snd_mixer_open(&lMixer, 0);
    require_noerr(err, exit);

    err = snd_mixer_attach(lMixer, "default");
    require_noerr(err, exit);

    err = snd_mixer_selem_register(lMixer, NULL, NULL);
    require_noerr(err, exit);

    err = snd_mixer_load(lMixer);
    require_noerr(err, exit);

    snd_mixer_selem_id_alloca(&sid);
    snd_mixer_selem_id_set_name(sid, streamInfo[stream_name][1]);
    snd_mixer_selem_id_set_index(sid, 0);

    lDuckElement = snd_mixer_find_selem(lMixer, sid);
    if (!lDuckElement) {
        goto exit;
    }
    gMixer = lMixer;
    gDuckElement = lDuckElement;
exit:

    return err;
}

void AudioControl::ALSAMixerClose()
{
    if (gMixer == NULL)
        return ;
    if (gMixer) {
        //snd_mixer_elem_detach(gDuckElement, );
        snd_mixer_close(gMixer);
        gMixer = NULL;
        gDuckElement = NULL;
    }
}


int AudioControl::Set(int Audio_Type, int nValue)
{
    snd_ctl_elem_id_t *pid = NULL;
    snd_ctl_elem_info_t *pinfo = NULL;
    snd_ctl_elem_value_t *pcontrol = NULL;
    snd_ctl_elem_id_alloca(&pid);
    snd_ctl_elem_info_alloca(&pinfo);
    snd_ctl_elem_value_alloca(&pcontrol);
    int type = 0, size = 0;
    if (getpid(Audio_Type, &pid) == 0) {
        if (getpinfo(pid, &pinfo) == 0) {
            getpcontrol(pid, pinfo, &pcontrol);
            get_type(pinfo, &type);
            get_size(pinfo, &size);
            set_value(pid, pinfo, pcontrol, nValue);
        }
    }
    return 0;
}

int AudioControl::Get(int Audio_Type, int *nValue)
{
    snd_ctl_elem_id_t *pid = NULL;
    snd_ctl_elem_info_t *pinfo = NULL;
    snd_ctl_elem_value_t *pcontrol = NULL;
    //snd_hctl_elem_t *pelem = NULL;
    snd_ctl_elem_id_alloca(&pid);
    snd_ctl_elem_info_alloca(&pinfo);
    snd_ctl_elem_value_alloca(&pcontrol);
    int type = 0, size = 0;
    int volume = 0;
    if(getpid(Audio_Type, &pid) == 0)
    {
        if(getpinfo(pid, &pinfo) == 0)
        {
            getpcontrol(pid, pinfo, &pcontrol);
            get_type(pinfo, &type);
            get_size(pinfo, &size);
            get_value(type, size, pcontrol, &volume);
            *nValue = volume;
        }
    }
    return 0;
}

int AudioControl::GetRange(int Audio_Type, int *pMin ,int *pMax)
{
    snd_ctl_elem_id_t *pid = NULL;
    snd_ctl_elem_info_t *pinfo = NULL;
    snd_ctl_elem_value_t *pcontrol = NULL;
    snd_hctl_elem_t *pelem = NULL;
    snd_ctl_elem_id_alloca(&pid);
    snd_ctl_elem_info_alloca(&pinfo);
    snd_ctl_elem_value_alloca(&pcontrol);
    int type = 0, size = 0;
    Data data;
    if(getpid(Audio_Type, &pid) == 0)
    {
        if(getpinfo(pid, &pinfo) == 0)
        {
            getpelem(pid, &pelem);
            getpcontrol(pid, pinfo, &pcontrol);
            get_type(pinfo, &type);
            get_size(pinfo, &size);
            //get_range(type, pelem, pinfo, &data);
            get_range(type, pinfo, &data);
            *pMin = data.min;
            *pMax = data.max;
        }
    }
    return 0;
}


int AudioControl::ArkALSADuckMixerGetVolume()
{
    int	err;
    //snd_mixer_selem_channel_id_t		channel;
    //int nIndex = (int)channel;
    int nIndex = 0;
    long minVolume, maxVolume, volume;

    err = snd_mixer_selem_get_playback_volume_range( gDuckElement, &minVolume, &maxVolume );
    if (!err) {

        for (nIndex = 0; nIndex < (int)SND_MIXER_SCHN_LAST; ++nIndex) {
            if (snd_mixer_selem_has_playback_channel( gDuckElement, (snd_mixer_selem_channel_id_t)nIndex)) {
                err = snd_mixer_selem_get_playback_volume(gDuckElement, (snd_mixer_selem_channel_id_t)nIndex, &volume);
                if (!err)
                    break;
            }
        }
    }

//exit:
    return (volume);
}

void AudioControl::ArkALSADuckMixerSetVolume(int inVolume)
{
    int       err;
    long      minVolume, maxVolume, newVolume;
    if (!gDuckElement) {
            printf("set gDuckElement == NULL\n");
            return ;
        }

    err = snd_mixer_selem_get_playback_volume_range( gDuckElement, &minVolume, &maxVolume );
    if (!err) {
        newVolume = Clamp( inVolume, minVolume, maxVolume );
        snd_mixer_selem_set_playback_volume_all( gDuckElement, newVolume );
    }
}

void AudioControl::Set_Plug_Softvol_Volume(int streamtype, int volume)
{
    int ret = 0;
   // const char *ctlname = NULL;
    long tmp = (long)volume;

    snd_mixer_t *handle = NULL;
    snd_mixer_selem_id_t *sid = NULL;
    snd_mixer_elem_t *elem = NULL;

    ret = snd_mixer_open(&handle, 0);
    if(ret < 0){
        qDebug()<<"@audio-ac snd mixer open error";
        return ;
    }

    ret = snd_mixer_attach(handle, "hw:0");
    if(ret < 0){
        qDebug()<<"@audio-ac snd mixer attach error";
        snd_mixer_close(handle);
        return ;
    }

    ret = snd_mixer_selem_register(handle, NULL, NULL);
    if(ret < 0){
        qDebug()<<"@audio-ac snd mixer selem register error";
        snd_mixer_close(handle);
        return ;
    }

    ret = snd_mixer_load(handle);
    if(ret < 0){
        qDebug()<<"@audio-ac snd mixer load error";
        snd_mixer_close(handle);
        return ;
    }

    snd_mixer_selem_id_alloca(&sid);
    snd_mixer_selem_id_set_index(sid, 0);
    snd_mixer_selem_id_set_name(sid, streamInfo[streamtype][1]);

    elem = snd_mixer_find_selem(handle, sid);
    if(!elem){
        qDebug()<<"@audio-ac snd mixer find selem error";
        snd_mixer_close(handle);
        return ;
    }

    long minv,maxv;
    long get_vol;
    snd_mixer_selem_get_playback_volume_range(elem, &minv, &maxv);
    ret = snd_mixer_selem_get_playback_volume(elem, (snd_mixer_selem_channel_id_t)0, &get_vol);
    if(ret < 0){
        qDebug()<<"@audio-ac snd mixer get playback volume error";
        snd_mixer_close(handle);
        return ;
    }

    if(tmp == get_vol){
        qDebug()<<"@audio-ac volume is already existed "<<get_vol<<" exit";
        snd_mixer_close(handle);
        return;
    }

    tmp = ((tmp < minv ? minv : tmp) > maxv ? maxv : tmp) ;

    if(snd_mixer_selem_has_playback_volume(elem)){
        qDebug()<<"@audio-ac set volume "<<streamInfo[streamtype][1]<<"  "<<tmp;
        ret = snd_mixer_selem_set_playback_volume_all(elem, tmp);
    }

    snd_mixer_close(handle);
}

/*


void AudioControl::Set_Plug_Mixer_Control(int ctrlNo, int value)
{
	
	qDebug("@audio-ctl ctrlNo:%s value:%d ", gCtrlName[ctrlNo], value);
	int err = 0;
	snd_hctl_t *hctl;
	err = snd_hctl_open(&hctl, "hw:0", 0);
	err = snd_hctl_load(hctl);
	
	//initialise snd_ctl_elem_id
	snd_ctl_elem_id_t *id;
	snd_ctl_elem_id_alloca(&id);
	snd_ctl_elem_id_set_interface(id, SND_CTL_ELEM_IFACE_MIXER);
	snd_ctl_elem_id_set_name(id, gCtrlName[ctrlNo]);

	snd_hctl_elem_t *elem = snd_hctl_find_elem(hctl, id);

	snd_ctl_elem_value_t *control;
	snd_ctl_elem_value_alloca(&control);
	snd_ctl_elem_value_set_id(control, id);   

	snd_ctl_elem_value_set_integer(control, 0, value);
	err = snd_hctl_elem_write(elem, control);
	if(!err){
		qDebug()<<"@audio-ctl Set_Plug_Mixer_Control error!";
	}
	snd_hctl_close(hctl);
}
*/

int AudioControl::getpid(int audio_type, snd_ctl_elem_id_t **ppid)
{
    char *paudio_id = getstring(audio_type);
    if (snd_ctl_ascii_elem_id_parse(*ppid, paudio_id))
    {
        printf("-- snd_ctl_ascii_elem_id_parse failed--\n");
        return -1;
    }
    return 0;
}

int AudioControl::getpinfo(snd_ctl_elem_id_t *pid, snd_ctl_elem_info_t **ppinfo)
{
    int err;
    snd_ctl_t *handle = NULL;

    if (handle == NULL && (err = snd_ctl_open(&handle, DEFAULT, 0)) < 0)
    {
        printf("-- snd_ctl_open failed--\n");
        return -1;
    }

    snd_ctl_elem_info_set_id(*ppinfo, pid);

    if ((err = snd_ctl_elem_info(handle, *ppinfo)) < 0)
    {
        snd_ctl_close(handle);
        handle = NULL;
        printf("-- snd_ctl_elem_info failed--\r\n");
        return -1;
    }

    snd_ctl_close(handle);
    handle = NULL;
    return 0;
}

int AudioControl::getpelem(snd_ctl_elem_id_t *pid, snd_hctl_elem_t **ppelem)
{
    int err;
    snd_hctl_t *hctl;
    if ((err = snd_hctl_open(&hctl, DEFAULT, 0)) < 0)
    {
        printf("-- snd_hctl_open failed--\n");
        return -1;
    }
    if ((err = snd_hctl_load(hctl)) < 0)
    {
        printf("-- snd_hctl_load failed--\n");
		snd_hctl_close(hctl);
        return -1;
    }

    *ppelem = snd_hctl_find_elem(hctl, pid);
    snd_hctl_close(hctl);
    return 0;
}

int AudioControl::getpcontrol(snd_ctl_elem_id_t *pid, snd_ctl_elem_info_t *pinfo ,snd_ctl_elem_value_t **ppcontrol)
{
    int err;
    snd_hctl_t *hctl;
    snd_hctl_elem_t *elem;
    if ((err = snd_hctl_open(&hctl, DEFAULT, 0)) < 0)
    {
        printf("-- snd_hctl_open failed--\n");
        return -1;
    }

    if ((err = snd_hctl_load(hctl)) < 0)
    {
        printf("-- snd_hctl_load failed--\n");
		snd_hctl_close(hctl);
        return -1;
    }
    elem = snd_hctl_find_elem(hctl, pid);
    if ((err = snd_hctl_elem_info(elem, pinfo)) < 0)
    {
        printf("-- snd_hctl_elem_info failed --\n");
		snd_hctl_close(hctl);
        return -1;
    }
    snd_ctl_elem_info_is_readable(pinfo);
    if ((err = snd_hctl_elem_read(elem, *ppcontrol)) < 0)
    {
        printf("-- snd_hctl_elem_read failed--\n");
		snd_hctl_close(hctl);
        return -1;
    }
    snd_hctl_close(hctl);
    return 0;
}

int AudioControl::get_type(snd_ctl_elem_info_t *pinfo, int *ptype)
{
    //int err;
    int type = -1;
    if ((type = snd_ctl_elem_info_get_type(pinfo)) < 0)
    {
        printf("-- snd_hctl_elem_read failed--\n");
        return -1;
    }
    *ptype = type;
    return 0;
}

int AudioControl::get_size(snd_ctl_elem_info_t *pinfo, int *psize)
{
    //int err;
    int size = -1;
    if ((size = snd_ctl_elem_info_get_count(pinfo)) < 0)
    {
        printf("-- snd_hctl_elem_read failed--\n");
        return -1;
    }
    *psize = size;
    return 0;
}

int AudioControl::get_range(int type, snd_ctl_elem_info_t *pinfo, Data *pdata)
{

    switch (type)
    {
    case SND_CTL_ELEM_TYPE_INTEGER:
        pdata->min = snd_ctl_elem_info_get_min(pinfo);
        pdata->max = snd_ctl_elem_info_get_max(pinfo);
        pdata->step = snd_ctl_elem_info_get_step(pinfo);
        break;
    case SND_CTL_ELEM_TYPE_INTEGER64:
        pdata->min = snd_ctl_elem_info_get_min(pinfo);
        pdata->max = snd_ctl_elem_info_get_max(pinfo);
        pdata->step = snd_ctl_elem_info_get_step(pinfo);
        break;
    default:
        pdata->min = 0;
        pdata->max = 0;
        pdata->step = 0;
        break;
    }
    return 0;
}

int AudioControl::get_value(int type, int count, snd_ctl_elem_value_t *pcontrol , int *pvalue)
{
    int value = 0;
    for (int index = 0; index < count; index++) {
        switch (type) {
        case SND_CTL_ELEM_TYPE_BOOLEAN:
            value = snd_ctl_elem_value_get_boolean(pcontrol, index);
            break;
        case SND_CTL_ELEM_TYPE_INTEGER:
            value = snd_ctl_elem_value_get_integer(pcontrol, index);
            //printf("integer value = %d\r\n", value);
            break;
        case SND_CTL_ELEM_TYPE_INTEGER64:
            value = snd_ctl_elem_value_get_integer64(pcontrol, index);
            break;
        case SND_CTL_ELEM_TYPE_ENUMERATED:
            value = snd_ctl_elem_value_get_enumerated(pcontrol, index);
            printf("enumerated value = %d\r\n", value);
            break;
        case SND_CTL_ELEM_TYPE_BYTES:
            value = snd_ctl_elem_value_get_byte(pcontrol, index);
            break;
        default:
            printf("?");
            break;
        }
    }
    *pvalue = value;
    return 0;
}

int AudioControl::set_value(snd_ctl_elem_id_t *pid, snd_ctl_elem_info_t *pinfo, snd_ctl_elem_value_t *pcontrol, int value)
{
    int err;
    snd_ctl_t *handle = NULL;
    if (handle == NULL && (err = snd_ctl_open(&handle, DEFAULT, 0)) < 0)
    {
        printf("-- snd_ctl_open failed--\n");
        return -1;
    }
    snd_ctl_elem_value_set_id(pcontrol, pid);

    if ((err = snd_ctl_elem_read(handle, pcontrol)) < 0)
    {
        snd_ctl_close(handle);
        handle = NULL;
        return -1;
    }
    char sz_value[20] = {0,};
    sprintf(sz_value, "%d", value);
    //printf("set value = %d\r\n", value);
    err = snd_ctl_ascii_value_parse(handle, pcontrol, pinfo, sz_value);
    if (err < 0)
    {
        snd_ctl_close(handle);
        handle = NULL;
        return  -1;
    }
    if ((err = snd_ctl_elem_write(handle, pcontrol)) < 0)
    {
        snd_ctl_close(handle);
        handle = NULL;
        return -1;
    }
    snd_ctl_close(handle);
    handle = NULL;

    return 0;
}

char* AudioControl::getstring(int type)
{
    static char szID[100] = {0,};
    switch (type) {
	    case EQ_PRESET:
	        strcpy(szID, EQ_PRESET_ID);
	        break;
	    case EQ_BAND0:
	        strcpy(szID, EQ_BAND0_ID);
	        break;
	    case EQ_BAND1:
	        strcpy(szID, EQ_BAND1_ID);
	        break;
	    case EQ_BAND2:
	        strcpy(szID, EQ_BAND2_ID);
	        break;
	    case EQ_BAND3:
	        strcpy(szID, EQ_BAND3_ID);
	        break;
	    case EQ_BAND4:
	        strcpy(szID, EQ_BAND4_ID);
	        break;
	    case EQ_BAND5:
	        strcpy(szID, EQ_BAND5_ID);
	        break;
		case EQ_BAND6:
	        strcpy(szID, EQ_BAND6_ID);
	        break;
		case MEDIA_VOL:
	        strcpy(szID, DSP_MEDIA_VOLID);
	        break;
		case MEDIA_MUTE:
	        strcpy(szID, DSP_MEDIA_MUTEID);
	        break;
		case RADIO_VOL:
	        strcpy(szID, DSP_RADIO_VOLID);
	        break;
		case RADIO_MUTE:
	        strcpy(szID, DSP_RADIO_MUTEID);
	        break;
		case SOFTMST0_VOL:
			strcpy(szID, SOFTMASTER0_ID);
			break;
		case SOFTMST1_VOL:
			strcpy(szID, SOFTMASTER1_ID);
			break;
		case SOFTMST2_VOL:
			strcpy(szID, SOFTMASTER2_ID);
			break;
		case SOFTMST3_VOL:
			strcpy(szID, SOFTMASTER3_ID);
			break;
		case SOFTMST4_VOL:
			strcpy(szID, SOFTMASTER4_ID);
			break;
		case SOFTMST5_VOL:
			strcpy(szID, SOFTMASTER5_ID);
			break;
		case SOFTMST6_VOL:
			strcpy(szID, SOFTMASTER6_ID);
			break;
		case SOFTMST12_VOL:
			strcpy(szID, SOFTMASTER12_ID);
			break;
		case SOFTMST8_VOL:
			strcpy(szID, SOFTMASTER8_ID);
			break;
		case SOFTMST11_VOL:
			strcpy(szID, SOFTMASTER11_ID);
			break;
		case SOFTMST13_VOL:
			strcpy(szID, SOFTMASTER13_ID);
			break;
		case SOFTMST14_VOL:
			strcpy(szID, SOFTMASTER14_ID);
			break;
		case ADC_MUTE:
			strcpy(szID, ADC_MUTE_ID);
	    default: {
	        break;
	    }
    }
    return szID;
}



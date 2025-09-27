/* @file  gps.c
   @brief source file to handle gps message sentences
   @author Shyam Jha (Avinashee Tech)
*/

#include "gps.h"
#include "ringbuffer.h"
#include "main.h"

uint16_t timeout_ms = 0;          //variable to set timeout in milliseconds
uint16_t UTCtoIST = 530;          //UTC+5.5hr = IST

/**
 * @brief  peek GPS buffer dataset
 * @param  None
 * @retval return 0 if buffer empty else return buffer data element
 */
char peek_dataset(void){

	char buffer_element;
	if(isbuffer_empty()){
		return 0;
	}else{
		buffer_peek(&buffer_element);
	}
	return buffer_element;

}

/**
 * @brief  find GPS sentence message
 * @param  None
 * @retval return 1 if complete GPS sentence found else return 0 after timeout
 * @note   find GPS sentence message characters presence in the GPS buffer
 *         character by character. Continue find until GPS sentence found
 *         or else trigger timeout after 500ms of search.
 */
uint8_t find_dataset(int dataset){

	char *dataset_initials;
	switch(dataset){
	case GGA:{
		        dataset_initials = "GGA";
	         }
		     break;
	case GLL:{
		        dataset_initials = "GLL";
	         }
	         break;
	case GSA:{
				dataset_initials = "GSA";
			 }
			 break;
	case GSV:{
				dataset_initials = "GSV";
			 }
			 break;
	case RMC:{
				dataset_initials = "RMC";
			 }
			 break;
	case VTG:{
				dataset_initials = "VTG";
			 }
			 break;
	case ZDA:{
		        dataset_initials = "ZDA";
	         }
	         break;
	default:
		     break;
	}

	char buffer_element;
	uint8_t dataset_index = 0;
	uint8_t found_dataset = 0;
	/*Start Finding GPS sentence message initials*/
    do{
    	timeout_ms = 500;         //500 milliseconds timeout set
    	while(isbuffer_empty()){  //wait here until data is available
    		if(timeout_ms==0){
    			return 0;         //timeout hit
    		}
    	}

        if(dataset_initials[dataset_index]==peek_dataset()){ //if character found
    		buffer_remove(&buffer_element);   //remove data element from buffer
            found_dataset = 2;                //indicates you found at least one character
    		dataset_index++;                  //increment index of dataset_initials(declared above) to find
    		if(dataset_index==strlen(dataset_initials)){  //if all characters found
    			found_dataset = 1;            //indicates you found exact match of characters
    			return found_dataset;
    		}
        }else{
        	if(found_dataset==2){            //if you found one character but then other characters differ
        		dataset_index = 0;           //start again
        		found_dataset = 0;
        	}

        	buffer_remove(&buffer_element);  //remove data element from buffer if character mismatch occurs
        }
    }while(found_dataset!=1);

    return found_dataset;

}

/**
 * @brief  save GPS sentence message
 * @param  buffertocopyinto data buffer to copy GPS sentence message
 * @retval return 1 if GPS sentence copy completed else return 0 after timeout
 * @note   save GPS sentence message characters in the GPS buffer provided
 *         until character '*' asterisk.
 *         trigger timeout after 500ms.
 */
uint8_t save_dataset(char *buffertocopyinto){

	char buffer_element;
	uint8_t saved_dataset = 0;
	uint8_t buffertocopyinto_idx = 0;;
	do{
		timeout_ms = 500;         //500 milliseconds timeout set
		while(isbuffer_empty()){  //wait here until data is available
			if(timeout_ms==0){
				return 0;         //timeout hit
			}
		}

        if(peek_dataset()!='*'){            //save dataset until * character
			buffer_remove(&buffer_element); //remove data element from buffer
			buffertocopyinto[buffertocopyinto_idx] = buffer_element; //copy into the dataset respective buffer
			buffertocopyinto_idx++;         //increment index
        }else{
        	saved_dataset = 1;
        	return 1;
        }


	}while(!saved_dataset);

	return 0;
}

/**
 * @brief  decode GGA sentence message
 * @param  gga_dataset pointer to GGA sentence dataset
 * @param  ublox_gps   pointer to main GPS dataset structure
 * @retval return 1 if GPS fix achieved else return 0
 * @note   decode GGA sentence from the available GGA dataset
 *         buffer and update location, time, GPS quality. Sat Count values.
 */
uint8_t decode_ggadataset(char *gga_dataset,gps_t *ublox_gps){

	const char* delimiter = ",";  //character ',' separates the data available in NMEA dataset
    char* data_buffer[15] = {NULL};
    uint8_t data_bufferidx = 0;

    //extract all the data from the available GGA dataset
    char* token = strtok(gga_dataset,delimiter);
    while(token!=NULL){
    	data_buffer[data_bufferidx] = token;
    	data_bufferidx++;
    	token = strtok(NULL,delimiter);
    }


    /**************Time**************/
    //UTC
    uint8_t hr = (atoi(data_buffer[0])/10000);
    uint8_t min = (atoi(data_buffer[0])/100)%100;
    uint8_t sec = (atoi(data_buffer[0])%100);

    //UTC to IST
    hr = hr + (UTCtoIST/100);
    min = min + (UTCtoIST%100);

    if(min>59){
		min = min - 60;
		hr++;
	}

    if(hr>=24){
    	hr = hr - 24;
    }

    //update time to main structure
    ublox_gps->gga.IST_time.hour = hr;
    ublox_gps->gga.IST_time.minute = min;
    ublox_gps->gga.IST_time.second = sec;

    /*************Location*************/
    //Latitude
    float lat = strtof(data_buffer[1],NULL)/100;
    char lat_direction = *(data_buffer[2]);

    //Longitude
    float lon = strtof(data_buffer[3],NULL)/100;
    char lon_direction = *(data_buffer[4]);

    //update location to main structure
	ublox_gps->gga.location.latitude = lat;
	ublox_gps->gga.location.NS = lat_direction;
	ublox_gps->gga.location.longitude = lon;
	ublox_gps->gga.location.EW = lon_direction;

    /***********GPS quality************/
	uint8_t gps_quality = atoi(data_buffer[5]);

	//update gps quality to main structure
	ublox_gps->gga.gps_fix_valid = gps_quality;

    /***Satellite count (used for position calc.)***/
	uint8_t sat_count = atoi(data_buffer[6]);

	//update sat count to main structure
	ublox_gps->gga.satellite_count = sat_count;

    /*************HDOP***************/
	float hdop = strtof(data_buffer[7],NULL);

	//update hdop to main structure
	ublox_gps->gga.hdop = hdop;

    /************Height**************/
    float alt = strtof(data_buffer[8],NULL);
    char unit = *(data_buffer[9]);

    //update altitude to main structure
	ublox_gps->gga.altitude.altitude = alt;
	ublox_gps->gga.altitude.unit = unit;

    //check if gps fix is valid
	if(gps_quality!=0){
		return 1;
	}

	return 0;

}


/**
 * @brief  decode GSV sentence message
 * @param  gsv_dataset pointer to GSV sentence dataset
 * @param  ublox_gps   pointer to main GPS dataset structure
 * @retval return 1 if GPS total satellite count valid else return 0
 * @note   decode GSV sentence from the available GSV dataset
 *         buffer and update satellite details, total sat in view values.
 */
uint8_t decode_gsvdataset(char *gsv_dataset,gps_t *ublox_gps){

	const char* delimiter = ",";  //character ',' separates the data available in NMEA dataset
    char* data_buffer[30] = {NULL};
    uint8_t data_bufferidx = 0;
    int sat_id = 0;
    int sat_elev = 0;
    int sat_azi = 0;
    int sat_snr = 0;

    /*extract all the data from the available GSV dataset*/
    char* token = strtok_function(gsv_dataset,delimiter);
    while(token){
    	data_buffer[data_bufferidx] = token;
    	data_bufferidx++;
    	token = strtok_function(NULL,delimiter);
    }

    /**********Total number of datasets**********/
    ublox_gps->gsv.total_datasets = atoi(data_buffer[1]);

    /**********Current number of dataset********/
    ublox_gps->gsv.current_dataset = atoi(data_buffer[2]);

    /*************Total sat in view*************/
    ublox_gps->gsv.total_num_sats = atoi(data_buffer[3]);

    /*************Satellite details*************/
    int idx = 4;
    uint8_t display_sats_div = 0;
    uint8_t display_sats_rem = 0;
    uint8_t display_sats = 0;
    if(ublox_gps->gsv.total_num_sats>=4){
    	display_sats_div = (ublox_gps->gsv.total_num_sats)/4;
    	display_sats_rem = (ublox_gps->gsv.total_num_sats)%4;

    	if(ublox_gps->gsv.current_dataset<=display_sats_div){
    		display_sats = 4;
    	}else{
    		display_sats = display_sats_rem;
    	}

    }else{
    	display_sats = ublox_gps->gsv.total_num_sats;
    }

    //Loop for total number of satellites
    for(int sat_index=0;sat_index<display_sats;sat_index++){
    	sat_id = atoi(data_buffer[idx]);      //satellite id
    	idx++;
    	sat_elev = atoi(data_buffer[idx]);    //satellite elevation
    	idx++;
    	sat_azi = atoi(data_buffer[idx]);     //satellite azimuth
    	idx++;
    	sat_snr = atoi(data_buffer[idx]);     //satellite SNR
    	idx++;

    	//update satellite data index according to dataset pages
		switch(ublox_gps->gsv.current_dataset){
		case 1:{
			       static int start_index=0;
			       ublox_gps->gsv.sat_id[start_index] = sat_id;
			       ublox_gps->gsv.sat_elev[start_index] = sat_elev;
			       ublox_gps->gsv.sat_azi[start_index] = sat_azi;
				   ublox_gps->gsv.sat_snr[start_index] = sat_snr;
				   start_index++;
				   if(start_index==4){
					   start_index = 0;
				   }
			   }
			   break;
		case 2:{
			       static int start_index=4;
				   ublox_gps->gsv.sat_id[start_index] = sat_id;
				   ublox_gps->gsv.sat_elev[start_index] = sat_elev;
				   ublox_gps->gsv.sat_azi[start_index] = sat_azi;
				   ublox_gps->gsv.sat_snr[start_index] = sat_snr;
				   start_index++;
				   if(start_index==8){
					   start_index = 4;
				   }
			   }
			   break;
		case 3:{
			       static int start_index=8;
				   ublox_gps->gsv.sat_id[start_index] = sat_id;
				   ublox_gps->gsv.sat_elev[start_index] = sat_elev;
				   ublox_gps->gsv.sat_azi[start_index] = sat_azi;
				   ublox_gps->gsv.sat_snr[start_index] = sat_snr;
				   start_index++;
				   if(start_index==12){
					   start_index = 8;
				   }
			   }
			   break;
		case 4:{
			       static int start_index=12;
				   ublox_gps->gsv.sat_id[start_index] = sat_id;
				   ublox_gps->gsv.sat_elev[start_index] = sat_elev;
				   ublox_gps->gsv.sat_azi[start_index] = sat_azi;
				   ublox_gps->gsv.sat_snr[start_index] = sat_snr;
				   start_index++;
				   if(start_index==16){
					   start_index = 12;
				   }
			   }
			   break;
		default:
			   break;
		}
    }



    //check if total satellite valid
	if(ublox_gps->gsv.total_num_sats!=0){
		return 1;
	}

	return 0;

}

/* @file  gps.h
   @brief header file to handle gps message sentences
   @author Shyam Jha (Avinashee Tech)
*/


#ifndef INC_GPS_H_
#define INC_GPS_H_

#include <stdint.h>
#include <stdlib.h>

//GPS NMEA protocol data set
enum{
	GGA,
	GLL,
	GSA,
	GSV,
	RMC,
	VTG,
	ZDA
};

/*Time structure*/
typedef struct {
	uint8_t hour;
	uint8_t minute;
	uint8_t second;
}Time_t;

/*Location structure*/
typedef struct {
	float latitude;
	char NS;
	float longitude;
	char EW;
}Location_t;

/*Altitude structure*/
typedef struct {
	float altitude;
	char unit;
}Altitude_t;

/*Date structure*/
typedef struct {
	uint8_t Day;
	uint8_t Month;
	uint16_t Year;
}Date_t;

/*GGA Sentence Message structure*/
typedef struct {
	Time_t IST_time;
	Location_t location;
	uint8_t gps_fix_valid;
	uint8_t satellite_count;
	float hdop;
    Altitude_t altitude;
}gga_t;

/*RMC Sentence Message structure*/
typedef struct {
	Time_t UTC_time;
	char isValid;
	Location_t location;
	float speed;
	float course;
	Date_t date;
}rmc_t;

/*GSV Sentence Message structure*/
typedef struct {
	uint8_t total_datasets;
	uint8_t current_dataset;
	uint8_t total_num_sats;
	int sat_id[20];
	int sat_elev[20];
	int sat_azi[20];
	int sat_snr[20];
}gsv_t;

/*GPS data structure*/
typedef struct {
	gga_t gga;
	rmc_t rmc;
	gsv_t gsv;
}gps_t;

/*function declarations*/
char peek_dataset(void);
uint8_t find_dataset(int dataset);
uint8_t save_dataset(char *buffertocopyinto);
uint8_t decode_ggadataset(char *gga_dataset,gps_t *ublox_gps);
uint8_t decode_gsvdataset(char *gsv_dataset,gps_t *ublox_gps);


#endif /* INC_GPS_H_ */

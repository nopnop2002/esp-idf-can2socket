typedef struct {
	uint8_t extd;			 	/**< Extended Frame Format (29bit ID) */
	uint8_t rtr;			 	/**< Message is a Remote Frame */
	uint32_t identifier;		/**< 11 or 29 bit identifier */
	uint8_t data_length_code;	/**< Data length code */
	uint8_t data[8];			/**< Data bytes (not relevant in RTR frame) */
} my_twai_frame_t;

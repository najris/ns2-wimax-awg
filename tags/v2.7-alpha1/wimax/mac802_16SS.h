/* This software was developed at the National Institute of Standards and
 * Technology by employees of the Federal Government in the course of
 * their official duties. Pursuant to title 17 Section 105 of the United
 * States Code this software is not subject to copyright protection and
 * is in the public domain.
 * NIST assumes no responsibility whatsoever for its use by other parties,
 * and makes no guarantees, expressed or implied, about its quality,
 * reliability, or any other characteristic.
 * <BR>
 * We would appreciate acknowledgement if the software is used.
 * <BR>
 * NIST ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS" CONDITION AND
 * DISCLAIM ANY LIABILITY OF ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING
 * FROM THE USE OF THIS SOFTWARE.
 * </PRE></P>
 * @author  rouil
 */

#ifndef MAC802_16SS_H
#define MAC802_16Ss_H

#include "mac802_16.h"

#define BS_NOT_CONNECTED -1 //bs_id when MN is not connected


/** Data structure to store MAC state */
struct state_info {
    Mac802_16State state;
    int bs_id;
    double frameduration;
    int frame_number;
    int channel;
    ConnectionManager * connectionManager;
    ServiceFlowHandler * serviceFlowHandler;
    struct peerNode *peer_list;
    int nb_peer;
};

/** The sub state (while connected) */
enum ss_sub_state {
    NORMAL,           //Normal state
    SCAN_PENDING,     //Normal period but pending scanning to start/resume
    SCANNING,         //Currently scanning
    HANDOVER_PENDING, //Normal state but handover to start
    HANDOVER          //Executing handover
};

/** Data structure to store scanning information */
struct scanning_structure {
    struct mac802_16_mob_scn_rsp_frame *rsp; //response from BS
    struct sched_state_info scan_state;     //current scanning state
    struct sched_state_info normal_state;   //backup of normal state
    int iteration;                          //current iteration
    WimaxScanIntervalTimer *scn_timer_;     //timer to notify end of scanning period
    int count;                              //number of frame before switching to scanning
    ss_sub_state substate;
    WimaxNeighborEntry *nbr; //current neighbor during scanning or handover
    //arrays of rdv timers
    WimaxRdvTimer *rdv_timers[2*MAX_NBR];
    int nb_rdv_timers;
    //handoff information
    int serving_bsid;
    int handoff_timeout; //number frame to wait before executing handoff
};

/**
 * Class implementing IEEE 802_16 State Machine at the SS
 */
class Mac802_16SS : public Mac802_16
{
    friend class BwRequest;

public:

    Mac802_16SS();

    /**
     * Interface with the TCL script
     * @param argc The number of parameter
     * @param argv The list of parameters
     */
    int command(int argc, const char*const* argv);

    /**
     * Set the mac state
     * @param state The new mac state
     */
    void setMacState (Mac802_16State state);

    /**
     * Return the mac state
     * @return The new mac state
     */
    Mac802_16State getMacState ();

    /**
     * Creates a snapshot of the MAC's state and reset it
     * @return The snapshot of the MAC's state
     */
    state_info *backup_state ();

    /**
     * Restore the state of the Mac
     * @param state The state to restore
     */
    void restore_state (state_info *state);

    /**
     * Process packets going out
     * @param p The packet to transmit
     */
    void transmit(Packet *p);

    inline u_char get_diuc() {
        return default_diuc_;
    }

    /**
     * Process packets going out
     * @param p The packet to transmit
     */
    void sendDown(Packet *p);

    /**
     * Process incoming packets
     * @param p The received packet
     */
    void sendUp(Packet *p);

    /*
     * Process the packet after receiving last bit
     */
    //void receive();

    /**
    * Process the packet after receiving last bit
    *@param p - the packet to be received  RPI
    */
    void receive(Packet *p);

    /* initial random propagation channel
     *rpi
     */
    int  GetInitialChannel();

// fille the frame with power values
    void addPowerinfo(hdr_mac802_16 *wimaxHdr,double power, bool collision );

    //chk for collision

    bool IsCollision (const hdr_mac802_16 *wimaxHdr,double power_subchannel);

#ifdef USE_802_21 //Switch to activate when using 802.21 modules (external package)
    /*
     * Configure/Request configuration
     * The upper layer sends a config object with the required
     * new values for the parameters (or PARAMETER_UNKNOWN_VALUE).
     * The MAC tries to set the values and return the new setting.
     * For examples if a MAC does not support a parameter it will
     * return  PARAMETER_UNKNOWN_VALUE
     * @param config The configuration object
     */
    void link_configure (link_parameter_config_t* config);

    /*
     * Disconnect from the PoA
     */
    void link_disconnect ();

    /*
     * Connect to the PoA
     * @param poa The address of PoA
     */
    void link_connect (int poa);

    /*
     * Set the operation mode
     * @param mode The new operation mode
     * @return true if transaction succeded
     */
    bool set_mode (mih_operation_mode_t mode);
#endif

    /*
     * Scan channel
     * @param req the scan request information
     */
    void link_scan (void *req);


    /*get and set Fast Feedback timer*/
    WimaxFfbTimer *  get_ffb_timer()  {
        return ffbTimer_;
    }
    void set_ffb_timer( WimaxFfbTimer  * ffb_timer) {
        ffbTimer_ = ffb_timer;
    }

    WimaxFfbTimer * ffbTimer_;

    double SINR_Avg_;

    /*CQICH channel related param*/
    int ffb_report_duration_;
    int ffb_report_period_;
    char ffb_report_type_;

    int cqich_alloc_offset_;

    /*Indicator to show if there is a cqich alloc ie.*/
    bool ss_cqich_alloc_ie_indicator_;
    bool CQICH_alloc_req_flag_;

    virtual void set_cqich_alloc_ie_ind(bool indicator) {
        printf("SS set to %d.\n", indicator);
        ss_cqich_alloc_ie_indicator_ = indicator;
    }
    virtual  bool get_cqich_alloc_ie_ind() {
        return ss_cqich_alloc_ie_indicator_;
    }


    void process_cqich_allocation_ie(mac802_16_ul_map_frame *frame);
    void send_FastFeedback_Report();
    void send_CQICH_alloc_request();


    /*Xingting added for FFB process in SS side.*/
    virtual void ffb_process();

    /*xingting set get modulation configure*/
    virtual bool get_registered_flag(int mac_index)  {
        return false;
    }
    virtual void set_registered_flag(int mac_index, bool reg)   {
        ;
    }
    virtual bool get_change_modulation_flag(int mac_index)   {
        return false;
    }
    virtual void set_change_modulation_flag(int mac_index, bool change)  {
        ;
    }
    virtual bool get_increase_modulation(int mac_index)  {
        return false;
    }
    virtual void set_increase_modulation(int mac_index, bool change)  {
        ;
    }

    //int get_effective_sinr() {return(16.0- ceil((INIT_SINR -effective_sinr_)/SINR_GRANULARITY)); } //hope to convert from SINR lognormal value to linear value 1~16
    double get_effective_sinr();//{return((short)(16.0- ceil((INIT_SINR -effective_sinr_)/SINR_GRANULARITY)));} //hope to convert from SINR lognormal value to linear value 1~16
    void set_effective_sinr(double sir) {
        effective_sinr_ = sir;
    }

    void set_channel_error(bool error_flag) {
        channel_error_rate_ = error_flag;
    }
    bool get_channel_error() {
        return channel_error_rate_;
    }

    void set_ss_current_mcs_index(int mcs_index) {
        current_mcs_index_ = mcs_index;
    }
    int get_ss_current_mcs_index() {
        return current_mcs_index_;
    }
    virtual void set_current_bler(float bler) {
        current_bler_= bler;
    }
    float get_current_bler() {
        return current_bler_;
    }
    virtual Packet * get_ffb_report() {
        return p_ffb_report_;
    }
    /*Need to set it to NULL because in sscheduler, a tmp pointer has pointed to the actual packet memory. Set it to NULL for sscheduler
    	to make sure the non-empty ffb report is a new one instead of the previous sent one.*/
    virtual void set_ffb_report_pointer_null();

    virtual void set_cqich_id(u_int16_t cqich_id) {
        cqich_id_ = cqich_id;
    }
    virtual u_int16_t get_cqich_id() {
        return cqich_id_;
    }

protected:
    int cqich_id_;

    /*this packet is to save the FFB report.*/
    Packet * p_ffb_report_;
    int current_mcs_index_;
    float current_bler_;

    double effective_sinr_;

    /*This variable is used to save the actual channel error status. Although the SINR might be very
    high, but the interference is also high and this packet might be corrupted. On the contary, the
    SINR might be low, the interference is low too and the packet might be successfully transmitted.*/
    bool channel_error_rate_;

    /*This variable is used to save anti_pingpong factor for AMC algorithm.*/
    int anti_pingpong_factor_;


    /**
     * init the timers and state
     */
    void init ();

    /**
     * Initialize default connection
     */
    void init_default_connections ();

    /**
     * Called when lost synchronization
     */
    void lost_synch ();

    /**
     * Start a new DL subframe
     */
    void start_dlsubframe ();

    /**
     * Start a new UL subframe
     */
    void start_ulsubframe ();

    /**
     * Start/Continue scanning
     */
    void resume_scanning ();

    /**
     * Pause scanning
     */
    void pause_scanning ();

    /**
     * Update the given timer and check if thresholds are crossed
     * @param watch the stat watch to update
     * @param value the stat value
     */
    void update_watch (StatWatch *watch, double value);

    /**
     * Update the given timer and check if thresholds are crossed
     * @param watch the stat watch to update
     * @param size the size of packet received
     */
    void update_throughput (ThroughputWatch *watch, double size);

#ifdef USE_802_21 //Switch to activate when using 802.21 modules (external package)
    /**
     * Poll the given stat variable to check status
     * @param type The link parameter type
     */
    void poll_stat (link_parameter_type_s type);

#endif

    /*Get and set fast feedback mode related parameters.*/
    inline int get_ffb_report_duration() {
        return ffb_report_duration_;
    }
    inline int get_ffb_report_period() {
        return ffb_report_period_;
    }
    inline char get_ffb_report_type() {
        return ffb_report_type_;
    }
    inline int get_ffb_report_cqich_id() {
        return cqich_id_;
    }
    inline void set_ffb_report_duration(int duration) {
        ffb_report_duration_ = duration;
    }
    inline void set_ffb_report_period(int period) {
        ffb_report_period_ = period;
    }
    inline void set_ffb_report_type(char type) {
        ffb_report_type_ = type;
    }
    inline void set_ffb_report_cqich_id(int cqich_id) {
        cqich_id_ = cqich_id;
    }


    /**
     * Called when a timer expires
     * @param The timer ID
     */
    virtual void expire (timer_id id);

private:

//Begin RPI
    /**
      * The function is used to process the MAC PDU when ARQ,Fragmentation and Packing are enabled
      * @param con The connection by which it arrived
      * @param p The packet to process
      */
    void process_mac_pdu_witharqfragpack (Connection *con, Packet *p);
//End RPI

    /**
     * Process a MAC type packet
     * @param p The packet to process
     */
    void process_mac_packet (Packet *p);

    /**
     * Process a DL_MAP message
     * @param frame The dl_map information
     */
    void process_dl_map (mac802_16_dl_map_frame *frame);

    /**
     * Process a DCD message
     * @param frame The dcd information
     */
    void process_dcd (mac802_16_dcd_frame *frame);

    /**
     * Process a UL_MAP message
     * @param frame The ul_map information
     */
    void process_ul_map (mac802_16_ul_map_frame *frame);

    /**
     * Process a UCD message
     * @param frame The ucd information
     */
    void process_ucd (mac802_16_ucd_frame *frame);

    /**
     * Process a ranging response message
     * @param frame The ranging response frame
     */
    void process_ranging_rsp (mac802_16_rng_rsp_frame *frame);

    /**
     * Process a registration response message
     * @param frame The registration response frame
     */
    void process_reg_rsp (mac802_16_reg_rsp_frame *frame);

    /**
     * Schedule a ranging
     */
    void init_ranging ();

    /**
     * Prepare to send a registration message
     */
    void send_registration ();

    /**
     * Send a scanning message to the serving BS
     */
    void send_scan_request ();

    /**
     * Process a scanning response message
     * @param frame The scanning response frame
     */
    void process_scan_rsp (mac802_16_mob_scn_rsp_frame *frame);

    /**
     * Process a BSHO-RSP message
     * @param frame The handover response frame
     */
    void process_bsho_rsp (mac802_16_mob_bsho_rsp_frame *frame);

    /**
     * Process a BSHO-RSP message
     * @param frame The handover response frame
     */
    void process_nbr_adv (mac802_16_mob_nbr_adv_frame *frame);

    /**
     * Send a MSHO-REQ message to the BS
     */
    void send_msho_req ();

    /**
     * Check rdv point when scanning
     */
    void check_rdv ();

    /**
     * Set the scan flag to true/false
     * param flag The value for the scan flag
     */
    void setScanFlag(bool flag);

    /**
     * return scan flag
     * @return the scan flag
     */
    bool isScanRunning();

    /**
     * A packet buffer used to temporary store a packet
     * received by upper layer. Used during scanning
     */
    Packet *pktBuf_;

    /*
     * The state of the MAC
     */
    Mac802_16State state_;

    /**
     * Current number of registration retry
     */
    u_int32_t nb_reg_retry_;

    /**
     * Current number of scan request retry
     */
    u_int32_t nb_scan_req_;

    /**
     * Timers
     */
    WimaxT1Timer  *t1timer_;
    WimaxT2Timer  *t2timer_;
    WimaxT6Timer  *t6timer_;
    WimaxT12Timer *t12timer_;
    WimaxT21Timer *t21timer_;
    WimaxLostDLMAPTimer *lostDLMAPtimer_;
    WimaxLostULMAPTimer *lostULMAPtimer_;
    WimaxT44Timer *t44timer_;

    /**
     * The scanning information
     */
    struct scanning_structure *scan_info_;

    /**
     * Indicates if scan has been requested
     */
    bool scan_flag_;

    /**
     * Default DIUC to use.
     */
    u_char default_diuc_;

    /**
    * Default UIUC to use.
    */
    u_char default_uiuc_;

};

#endif //MAC802_16SS_H


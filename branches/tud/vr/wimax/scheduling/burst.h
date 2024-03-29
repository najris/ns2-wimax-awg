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

#ifndef BURST_H
#define BURST_H

#include "queue.h"
#include "packet.h"

class PhyPdu;

class Burst;
LIST_HEAD (burst, Burst);

/**
 * This class describes a burst
 */
class Burst
{
    friend class WimaxBurstTimer;

public:
    /**
     * Creates a burst
     * @param phypdu The PhyPdu where it is located
     */
    Burst (PhyPdu *phypdu);

    /**
     * Delete the object
     */
    ~Burst ();

    /**
     * Return the CID for this burst
     * @return the CID for this burst
     */
    int getCid( );

    /**
     * Get and set cdma_code and opportunity from/to burst
     * @return the CID for this burst
     */
    inline short int getBurstCdmaCode( ) {
        return burstCdmaCode_;
    }
    inline short int getBurstCdmaTop() {
        return burstCdmaTop_;
    }
    inline void setBurstCdmaCode( short int cdmaCode ) {
    	burstCdmaCode_ = cdmaCode;
    }
    inline void setBurstCdmaTop( short int cdmaTop ) {
    	burstCdmaTop_ = cdmaTop;
    }


    /*Get and set the CQICH parameters*/
    char ext_uiuc_;
    u_int16_t cqich_alloc_ie_cqich_id_;
    u_int16_t ss_mac_id_;
    int cqich_alloc_offset_;
    int cqich_period_;
    int cqich_duration_;
    char cqich_fb_type_;
    //u_int16_t burst_ss_mac_id_;
    void set_cqich_ext_uiuc(char ext_uiuc) {
        ext_uiuc_ = ext_uiuc;
    }
    char get_cqich_ext_uiuc() {
        return ext_uiuc_;
    }

    /*These two functions are used by bsscheduler to allocate CQICH_Alloc_IE to SS.
        By using these two functions, UL burst can be used to transmit the allocated
        CQICH_id to the SS.*/
    virtual void set_cqich_id(u_int16_t id) {};
    virtual u_int16_t get_cqich_id() { return 0; }; 

    void set_cqich_alloc_offset(int offset) {
        cqich_alloc_offset_ = offset;
    }
    int get_cqich_alloc_offset() {
        return cqich_alloc_offset_;
    }

    void set_cqich_period(int period) {
        cqich_period_ = period;
    }
    int get_cqich_period() {
        return cqich_period_;
    }

    void set_cqich_duration(int duration) {
        cqich_duration_ = duration;
    }
    int get_cqich_duration() {
        return cqich_duration_;
    }

    void set_cqich_fb_type(char type) {
        cqich_fb_type_=type;
    }
    char get_cqich_fb_type() {
        return cqich_fb_type_;
    }

    virtual void setCqichSlotFlag(bool CqichSlotFlag) {};
    virtual bool getCqichSlotFlag() { return false; };

    virtual void setBurstSsMacId(u_int16_t ss_mac_id) {};
    virtual u_int16_t getBrustSsMacId() { return 0; };

    /**
     * Return the burst duration in units of OFDM symbols
     * @return the burst duration
     */
    int getDuration( );

    /**
     * Set the duration of the burst in units of OFDM symbols
     * @param duration The burst duration
     */
    void setDuration (int duration);

    /**
     * Return the burst start time in units of symbol duration
     * @return the burst start time
     */
    int getStarttime( );

    /**
     * Return the Interval Usage Code
     * @return the burst start time
     */
    int getIUC( );

//rpi add for OFDMA

    /**
     * return the subchannel offset
     */
    int getSubchannelOffset ();

    /**
     * return the number of subchannels
     */
    int getnumSubchannels ();

    /**
     * set the number of subchannels
     */
    void setnumSubchannels (int numsubchannels);

    /**
     * set the subchannel offset- @param subchannel offset
     */
    void setSubchannelOffset (int subchanneloffset);

// rpi end add for OFDMA

    /**
     * Set burst CID
     * @param cid The burst CID
     */
    void setCid( int cid );

    /**
     * Set burst start time in units of symbol duration
     * @param starttime the burst start time
     */
    void setStarttime( int starttime );

    /**
     * Set burst IUC
     * @param iuc The burst IUC
     */
    void setIUC( int iuc );

    /**
     * Enqueue a packet to be schedule in this burst
     * @param p The packet to queue
     */
    void enqueue (Packet *p);

    /**
     * Dequeue a packet from the queue
     * @param p The packet to enqueue
     */
    Packet * dequeue ();

    /**
     * Return the queue size in bytes
     */
    int getQueueLength();// { return queue_->byteLength();}


    /**
     * Remove the given packet from the queue
     * @param p The packet to remove
     */
    void remove(Packet* p);

    //rpi added for dlsubframe timer

    Packet * lookup (int n);

    int getQueueLength_packets();

    // rpi

    /**
     * Schedule the timer for this burst
     * @param time The time the trigger expires
     */
    //void trigger_timer (double time);

    // Chain element to the list
    inline void insert_entry_head(struct burst *head) {
        LIST_INSERT_HEAD(head, this, link);
    }

    // Chain element to the list
    inline void insert_entry(Burst *elem) {
        LIST_INSERT_AFTER(elem, this, link);
    }

    // Return next element in the chained list
    Burst* next_entry(void) const {
        return link.le_next;
    }

    // Remove the entry from the list
    inline void remove_entry() {
        LIST_REMOVE(this, link);
    }

protected:
    /**
     * A timer use to transmit the burst
     */
    //WimaxBurstTimer timer_;

    /**
     * Packets to be sent during this burst
     */
    PacketQueue* queue_;

    /**
     * Return the PhyPdu
     * @return the PhyPdu
     */
    inline PhyPdu *getPhyPdu () {
        return phypdu_;
    }

    /*
     * Pointer to next in the list
     */
    LIST_ENTRY(Burst) link;
    //LIST_ENTRY(Burst); //for magic draw

private:
    /**
     * The CID for the burst. If a broadcast or multicast is used, then Mac SDUs for different SSs can be included in the burst.
     */
    int cid_;

    /**
     * The burst duration in units of OFDM Symbols
     */
    int duration_;

    /**
     * The start time of the burst in units of Symbol Duration
     */
    int starttime_;

    /**
     * The profile ID
     */
    int iuc_;

    /**
     * The PhyPdu where it is located
     */
    PhyPdu *phypdu_;

// rpi addition for ofdma

    /**
     *  subchannel offset-
     */
    int subchannelOffset_;

    /**
     * number of subchannels
     */
    int numsubchannels_;

    /**
     * cdma code / -1 default value
     * and top
     */
    short int burstCdmaCode_;

    /*
     * cdma top ( used cdma slot) / -1 default value
     */
    short int burstCdmaTop_;
};

#endif

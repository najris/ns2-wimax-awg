/*
 * schedulingalgodualequalfill.cc
 *
 *  Created on: 08.08.2010
 *      Author: richter
 */

#include "schedulingalgodualequalfill.h"

#include "virtualallocation.h"
#include "connection.h"
#include "packet.h"

SchedulingAlgoDualEqualFill::SchedulingAlgoDualEqualFill() {
	// Initialize member variables
	nextConnectionPtr_ = NULL;

}

SchedulingAlgoDualEqualFill::~SchedulingAlgoDualEqualFill() {
	// TODO Auto-generated destructor stub
}

void SchedulingAlgoDualEqualFill::scheduleConnections( VirtualAllocation* virtualAllocation, int freeSlots)
{
	/* Calculate the number of data connections, the number of wanted Mrtr Bytes and the sum of wanted Mstr Bytes
	 * This could be done more efficient in bsscheduler.cc, but this would be a algorithm specific extension and
	 * would break the algorithm independent interface.
	 */

	// update totalNbOfSlots_ for statistic
	assert( ( 0 < movingAverageFactor_) && ( 1 > movingAverageFactor_) );
	totalNbOfSlots_ = ( totalNbOfSlots_ * ( 1 - movingAverageFactor_)) + ( freeSlots * movingAverageFactor_);

	int nbOfMrtrConnections = 0;
	int nbOfMstrConnections = 0;
	int nbOfUnscheduledConnections = 0;
	u_int32_t sumOfWantedMrtrBytes = 0;
	u_int32_t sumOfWantedMstrBytes = 0;

	// count number of allocated slots for mrtr demands
	int mrtrSlots = 0;
	// count the slots used to fulfill MSTR demands
	int mstrSlots = 0;


	// check if any connections have data to send
	if ( virtualAllocation->firstConnectionEntry()) {

		// run ones through whole the map
		do {
			if ( virtualAllocation->getConnection()->getType() == CONN_DATA ) {

				// Debugging propose
				// MRTR size is always less or equal to MSTR size
				assert( virtualAllocation->getWantedMrtrSize() <= virtualAllocation->getWantedMstrSize());

				// count the connection and the amount of data which has to be scheduled to fullfill the mrtr rates
				if ( virtualAllocation->getWantedMrtrSize() > 0 ) {

					nbOfMrtrConnections++;
					sumOfWantedMrtrBytes += virtualAllocation->getWantedMrtrSize();
				}

				// count the connection and the amount of data which can be schedules due to the mstr rates
				if ( (virtualAllocation->getWantedMstrSize() -  virtualAllocation->getWantedMrtrSize()) > 0) {

					nbOfMstrConnections++;
					sumOfWantedMstrBytes += ( virtualAllocation->getWantedMstrSize() - virtualAllocation->getWantedMrtrSize());
				}

				// count connections to be scheduled
				if ( virtualAllocation->getWantedMstrSize() > 0) {
					nbOfUnscheduledConnections++;
				}

				//printf("Connection CID %d Demand MRTR %d MSTR %d \n", virtualAllocation->getConnection()->get_cid(), virtualAllocation->getWantedMrtrSize(), virtualAllocation->getWantedMstrSize());
			}
		} while ( virtualAllocation->nextConnectionEntry() );


		/*
		 * Allocation of Slots for fulfilling the MRTR demands
		 */

		// for debugging purpose
		bool test;

		// get first data connection
		if ( nextConnectionPtr_ != NULL ) {
			// find last connection
			if ( ! virtualAllocation->findConnectionEntry( nextConnectionPtr_)) {
				// connection not found -> get first connection
				test = virtualAllocation->firstConnectionEntry();
				assert( test);
			}
		} else {
			// get first connection
			test = virtualAllocation->firstConnectionEntry();
			assert( test);
		}


		while ( ( nbOfMrtrConnections > 0 ) && ( freeSlots > 0 ) ) {

			// number of Connections which can be served in this iteration
			int conThisRound = nbOfMrtrConnections;

			int nbOfSlotsPerConnection = 0;

			if ( virtualAllocation->getBroadcastSlotCapacity() != 0) {
				// Downlink direction --> reserve space for dl-map
				if ( nbOfMrtrConnections < nbOfUnscheduledConnections) {
					// divide slots equally for the first round
					nbOfSlotsPerConnection = (freeSlots - ( ceil( double(nbOfMrtrConnections * DL_MAP_IE_SIZE) / virtualAllocation->getBroadcastSlotCapacity() ))) / nbOfMrtrConnections;
				} else {
					// divide slots equally for the first round
					nbOfSlotsPerConnection = (freeSlots - ( ceil( double( nbOfUnscheduledConnections * DL_MAP_IE_SIZE) / virtualAllocation->getBroadcastSlotCapacity() ))) / nbOfMrtrConnections;
				}
			} else {
				// Uplink Direction
				nbOfSlotsPerConnection = freeSlots / nbOfMrtrConnections;
			}

			// only one slot per connection left
			if ( nbOfSlotsPerConnection <= 0 ) {
				nbOfSlotsPerConnection = 1;
			}


			while ( ( conThisRound > 0) && ( freeSlots > 0) ) {

				// go to connection which has unfulfilled  Mrtr demands
				while ( ( virtualAllocation->getConnection()->getType() != CONN_DATA ) ||
						( virtualAllocation->getWantedMrtrSize() <= u_int32_t( virtualAllocation->getCurrentMrtrPayload() ) ) )  {
					// next connection

					// TODO: may cause problems ugly
					virtualAllocation->nextConnectionEntry();
				}

				// this connection gets up to nbOfSlotsPerConnection slots

				// calculate the corresponding number of bytes
				int allocatedSlots = nbOfSlotsPerConnection + virtualAllocation->getCurrentNbOfSlots();
				// increse slots for QPSK 1/2 to allow scheduling
				if ((allocatedSlots < 2 ) && (freeSlots > 1) && (virtualAllocation->getSlotCapacity() == 6)) {
					allocatedSlots = 2;
				}
				int maximumBytes = allocatedSlots * virtualAllocation->getSlotCapacity();


				// get fragmented bytes to calculate first packet size
				int fragmentedBytes = virtualAllocation->getConnection()->getFragmentBytes();


				// get first packed
				Packet * currentPacket = virtualAllocation->getConnection()->get_queue()->head();
				int allocatedBytes = 0;
				int allocatedPayload = 0;
				int wantedMrtrSize = virtualAllocation->getWantedMrtrSize();

				bool allocationDone = false;

				while ( ( currentPacket != NULL) && ( !allocationDone) ) {

					int packetPayload = HDR_CMN(currentPacket)->size() - fragmentedBytes;

					// check for sufficient space for headers
					int maxNewAllocation = maximumBytes - allocatedBytes;
					if (maxNewAllocation > HDR_MAC802_16_SIZE ) {
						// Sufficient space for PDUs
						if (maxNewAllocation > HDR_MAC802_16_SIZE + HDR_MAC802_16_FRAGSUB_SIZE ) {
							// Sufficient space for fragmented PDUs

							// allocated packet
							allocatedPayload += packetPayload;

							// check if MRTR was reached
							if (allocatedPayload > wantedMrtrSize) {
								// reduce allocation
								// fragmentation necessary
								fragmentedBytes = packetPayload - (allocatedPayload - wantedMrtrSize);
								allocatedBytes += packetPayload - (allocatedPayload - wantedMrtrSize) + HDR_MAC802_16_SIZE + HDR_MAC802_16_FRAGSUB_SIZE;
								allocatedPayload = wantedMrtrSize;
								// finish
								allocationDone = true;
							} else if ( fragmentedBytes > 0) {
								// a fragment is send
								allocatedBytes += packetPayload + HDR_MAC802_16_SIZE + HDR_MAC802_16_FRAGSUB_SIZE;
							} else {
								// entire packet is send
								allocatedBytes += packetPayload + HDR_MAC802_16_SIZE;
							}


							// check if maximum Bytes was reached
							if (allocatedBytes > maximumBytes) {
								if (fragmentedBytes > 0) {
									// last packet already fragmented --> fragment becomes smaller
									allocatedPayload -= (allocatedBytes - maximumBytes);
								} else {
									// new fragmentation necessary --> add fragmentation header
									allocatedPayload -= (allocatedBytes - maximumBytes) + HDR_MAC802_16_FRAGSUB_SIZE;
								}
								allocatedBytes = maximumBytes ;
								// finish
								allocationDone = true;

							}

							// check if demand is fullfilled
							if (allocatedPayload == wantedMrtrSize) {
								allocationDone = true;
								// reduce number of connection with mrtr demand
								nbOfMrtrConnections--;
								// avoids, that connection is called again
								virtualAllocation->updateWantedMrtrMstr( 0, virtualAllocation->getWantedMstrSize());
							}

							// fragment already handeled
							fragmentedBytes = 0;

						} else if ((fragmentedBytes == 0) && ( packetPayload <= HDR_MAC802_16_FRAGSUB_SIZE)){
							// add micro packet with 1 or 2 Bytes
							allocatedPayload += packetPayload;
							allocatedBytes += packetPayload + HDR_MAC802_16_SIZE;
							// finish this round
							allocationDone = true;
						}



					} else {
						// not sufficent space to schedule
						allocationDone = true;
					}

				// get next packet
				currentPacket = currentPacket->next_;
				// end loop through packets
				}

				// Calculate Allocated Slots
				allocatedSlots = int( ceil( double(allocatedBytes) / virtualAllocation->getSlotCapacity()) );


				int oldSlots = virtualAllocation->getCurrentNbOfSlots();
				if (( oldSlots == 0 ) && (virtualAllocation->getBroadcastSlotCapacity() != 0 )) {
					// new dl-map ie is needed

					// debug for QPSK 1/2
					assert( freeSlots >= 1);
					// increase broadcast burst
					freeSlots -= virtualAllocation->increaseBroadcastBurst( DL_MAP_IE_SIZE);

					// debug
					assert( freeSlots >= 0);

					// decrease unscheduled connections
					nbOfUnscheduledConnections--;
				}


				int newSlots = ( allocatedSlots - oldSlots);

				// debug
				// printf(" %d new Mrtr Slots for Connection CID %d \n", newSlots, virtualAllocation->getConnection()->get_cid() );

				// update freeSlots
				freeSlots -= newSlots;
				// update mrtrSlots
				mrtrSlots += newSlots;

				// check for debug
				assert( freeSlots >= 0);

				// update container
				virtualAllocation->updateAllocation( allocatedBytes, allocatedSlots,  u_int32_t( allocatedPayload), u_int32_t( allocatedPayload));

				// decrease loop counter
				conThisRound--;

				// get next connection
				virtualAllocation->nextConnectionEntry();
				nextConnectionPtr_ = virtualAllocation->getConnection();
			}
		}

		// check if there are free Slots after Mrtr allocation
		if ( freeSlots > 0) {

			// count mstr connections
			nbOfMstrConnections = 0;
			virtualAllocation->firstConnectionEntry();
			// run ones through the whole map
			do {
				if ( virtualAllocation->getConnection()->getType() == CONN_DATA ) {

					// Debugging propose
					// MRTR size is always less or equal to MSTR size
					assert( virtualAllocation->getWantedMrtrSize() <= virtualAllocation->getWantedMstrSize());

					// count the connection and the amount of data which can be schedules due to the mstr rates
					if ( (virtualAllocation->getWantedMstrSize() >  virtualAllocation->getCurrentMrtrPayload())) {

						nbOfMstrConnections++;
					}
				}
			} while ( virtualAllocation->nextConnectionEntry() );

			// get next connection to be served
			if ( nextConnectionPtr_ != NULL ) {
				// find last connection
				if ( ! virtualAllocation->findConnectionEntry( nextConnectionPtr_)) {
					// connection not found -> get first connection
					virtualAllocation->firstConnectionEntry();
				}
			} else {
				// get first connection
				virtualAllocation->firstConnectionEntry();
			}

			/*
			 * Allocation of Slots for fulfilling the MSTR demands
			 */

			while ( ( nbOfMstrConnections > 0 ) && ( freeSlots > 0 ) ) {

				// number of Connections which can be served in this iteration
				int conThisRound = nbOfMstrConnections;

				int nbOfSlotsPerConnection = 0;

				// divide slots equally for the first round
				if ( virtualAllocation->getBroadcastSlotCapacity() != 0) {
					// Downlink direction --> reserve space for dl-map
					//int nbOfSlotsPerConnection = freeSlots / nbOfMstrConnections;
					nbOfSlotsPerConnection = (freeSlots - ( ceil( double( nbOfUnscheduledConnections * DL_MAP_IE_SIZE) / virtualAllocation->getBroadcastSlotCapacity() ))) / nbOfMstrConnections;
				} else {
					// Uplink direction
					nbOfSlotsPerConnection = freeSlots / nbOfMstrConnections;
				}

				// only one slot per connection left
				if ( nbOfSlotsPerConnection <= 0 ) {
					nbOfSlotsPerConnection = 1;
				}

				while ( ( conThisRound > 0) && ( freeSlots > 0 ) ) {

					int i = 0;
					// go to connection which has unfulfilled  Mstr demands
					while  ( ( virtualAllocation->getConnection()->getType() != CONN_DATA ) 	||
							( ( virtualAllocation->getWantedMstrSize() <= u_int32_t( virtualAllocation->getCurrentMstrPayload()) ) && (i < 3 )) ){

						// next connection

						if ( ! virtualAllocation->nextConnectionEntry() ) {
							// 	count loops due to rounding errors
							i++;
						}
						// debug
						assert(i < 3);
					}

					// this connection gets up to nbOfSlotsPerConnection slots

					// calculate the corresponding number of bytes
					int allocatedSlots = nbOfSlotsPerConnection + virtualAllocation->getCurrentNbOfSlots();
					int maximumBytes = allocatedSlots * virtualAllocation->getSlotCapacity();

					// get fragmented bytes to calculate first packet size
					int fragmentedBytes = virtualAllocation->getConnection()->getFragmentBytes();


					// get first packed
					Packet * currentPacket = virtualAllocation->getConnection()->get_queue()->head();
					int allocatedBytes = 0;
					int allocatedPayload = 0;
					int wantedMstrSize = virtualAllocation->getWantedMstrSize();

					bool allocationDone = false;

					while ( ( currentPacket != NULL) && ( !allocationDone) ) {

						int packetPayload = HDR_CMN(currentPacket)->size() - fragmentedBytes;

						// check for sufficient space for headers
						int maxNewAllocation = maximumBytes - allocatedBytes;
						if (maxNewAllocation > HDR_MAC802_16_SIZE ) {
							// Sufficient space for PDUs
							if (maxNewAllocation > HDR_MAC802_16_SIZE + HDR_MAC802_16_FRAGSUB_SIZE ) {
								// Sufficient space for fragmented PDUs

								// allocated packet
								allocatedPayload += packetPayload;

								// check if MRTR was reached
								if (allocatedPayload > wantedMstrSize) {
									// reduce allocation
									// fragmentation necessary
									fragmentedBytes = packetPayload - (allocatedPayload - wantedMstrSize);
									allocatedBytes += packetPayload - (allocatedPayload - wantedMstrSize) + HDR_MAC802_16_SIZE + HDR_MAC802_16_FRAGSUB_SIZE;
									allocatedPayload = wantedMstrSize;
									// finish
									allocationDone = true;
								} else if ( fragmentedBytes > 0) {
									// a fragment is send
									allocatedBytes += packetPayload + HDR_MAC802_16_SIZE + HDR_MAC802_16_FRAGSUB_SIZE;
								} else {
									// entire packet is send
									allocatedBytes += packetPayload + HDR_MAC802_16_SIZE;
								}


								// check if maximum Bytes was reached
								if (allocatedBytes > maximumBytes) {
									if (fragmentedBytes > 0) {
										// last packet already fragmented --> fragment becomes smaller
										allocatedPayload -= (allocatedBytes - maximumBytes);
									} else {
										// new fragmentation necessary --> add fragmentation header
										allocatedPayload -= (allocatedBytes - maximumBytes) + HDR_MAC802_16_FRAGSUB_SIZE;
									}
									allocatedBytes = maximumBytes ;
									// finish
									allocationDone = true;

								}

								// check if demand is fullfilled
								if (allocatedPayload == wantedMstrSize) {
									allocationDone = true;
									// reduce number of connection with mstr demand
									nbOfMstrConnections--;
									// avoids, that connection is called again
									virtualAllocation->updateWantedMrtrMstr( 0, 0);
								}

								// fragment already handeled
								fragmentedBytes = 0;

							} else if ((fragmentedBytes == 0) && ( packetPayload <= HDR_MAC802_16_FRAGSUB_SIZE)){
								// add micro packet with 1 or 2 Bytes
								allocatedPayload += packetPayload;
								allocatedBytes += packetPayload + HDR_MAC802_16_SIZE;
								// finish this round
								allocationDone = true;
							}


						} else {
							// not sufficent space to schedule
							allocationDone = true;
						}

						// get next packet
						currentPacket = currentPacket->next_;

					// end loop through packets
					}

					// Calculate Allocated Slots
					allocatedSlots = int( ceil( double(allocatedBytes) / virtualAllocation->getSlotCapacity()) );


					int oldSlots = virtualAllocation->getCurrentNbOfSlots();
					if (( oldSlots == 0 ) && (virtualAllocation->getBroadcastSlotCapacity() != 0 )) {
						// new dl-map ie is needed

						// debug for QPSK 1/2
						assert( freeSlots >= 1);
						// increase broadcast burst
						freeSlots -= virtualAllocation->increaseBroadcastBurst( DL_MAP_IE_SIZE);

						// debug
						assert( freeSlots >= 0);

						// decrease unscheduled connections
						nbOfUnscheduledConnections--;
					}

					// calculate new assigned slots
					int newSlots = ( allocatedSlots - oldSlots);

					// debug
					// printf(" %d new Mstr Slots for Connection CID %d \n", newSlots, virtualAllocation->getConnection()->get_cid() );

					// check if connection can be scheduled
					if ( freeSlots >= newSlots) {

						// update freeSlots
						freeSlots -= newSlots;

						// update mstrSlots
						mstrSlots += newSlots;

						u_int32_t allocatedMrtrPayload = virtualAllocation->getCurrentMrtrPayload();

						// update container
						virtualAllocation->updateAllocation( allocatedBytes, allocatedSlots,  allocatedMrtrPayload, u_int32_t(allocatedPayload));
					} else {
						// reduce number of connections to avoid endless loops
						nbOfMstrConnections--;
					}
					// check for debug
					assert( freeSlots >= 0);
					assert( allocatedPayload >= 0);


					// decrease loop counter
					conThisRound--;

					// get next connection
					virtualAllocation->nextConnectionEntry();
					nextConnectionPtr_ = virtualAllocation->getConnection();
				}

			} // END Mstr allocation loop

		} // END: check if there are free Slots after Mrtr allocation

	} // END: check if any connections have data to send


	// update usedMrtrSlots_ for statistic
	usedMrtrSlots_ = ( usedMrtrSlots_ * ( 1 - movingAverageFactor_)) + ( mrtrSlots * movingAverageFactor_);
	// update usedMstrSlots_ for statistic
	usedMstrSlots_ = ( usedMstrSlots_ * ( 1 - movingAverageFactor_)) + ( mstrSlots * movingAverageFactor_);

}
/*
 * ------------------------------------------
 *
 *  HIGH-PERFORMANCE INTEGRATED MODELLING SYSTEM (HiPIMS)
 *  Luke S. Smith and Qiuhua Liang
 *  luke@smith.ac
 *
 *  School of Civil Engineering & Geosciences
 *  Newcastle University
 * 
 * ------------------------------------------
 *  This code is licensed under GPLv3. See LICENCE
 *  for more information.
 * ------------------------------------------
 *  Executor Control Base Class
 * ------------------------------------------
 *
 */
#ifndef HIPIMS_DOMAIN_CEXECUTORCONTROL_H_
#define HIPIMS_DOMAIN_CEXECUTORCONTROL_H_





/*
 *  EXECUTOR CONTROL CLASS
 *  CExecutorControl
 *
 *  Controls the model execution
 */
class CExecutorControl
{

	public:

		CExecutorControl( void );										
		virtual ~CExecutorControl( void );								

		// Public functions
		bool						isReady( void );					// Is the executor ready?
		void						setDeviceFilter( unsigned int );	// Filter to specific types of device
		unsigned int				getDeviceFilter();					// Fetch back the current device filter
		virtual bool				createDevices(void) = 0;			// Creates new classes for each device

		// Static functions
		static CExecutorControl*	createExecutor( unsigned char, CModel*);	// Create a new executor of the specified type
		
	protected:

		// Protected functions
		void					setState( unsigned int );				// Set the ready state
		unsigned int			deviceFilter;							// Device filter active for this executor

	private:

		// Private variables
		unsigned int			state;									// Ready state value

};

#endif

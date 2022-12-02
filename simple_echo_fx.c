/** @file simple_echo_fx.c
 *
 *
 * compile with = gcc -o simple_client simple_client.c `pkg-config --cflags --libs jack`
 */

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <jack/jack.h>

jack_port_t *input_port;
jack_port_t *output_port;
jack_client_t *client;

jack_default_audio_sample_t buffer [44100*1024];
jack_default_audio_sample_t mixedSignal [1024];
unsigned int bufferIndex = 0;

//Echo FX Parameter
unsigned int echoDelay = 15;
unsigned int feedBackDelay = 2;

/**
 * The process callback for this JACK application is called in a
 * special realtime thread once for each audio cycle.
 *
 * This client does nothing more than copy data from its input
 * port to its output port. It will exit when stopped by
 * the user (e.g. using Ctrl-C on a unix-ish operating system)
 */
int process(jack_nframes_t nframes, void *arg)
{
	jack_default_audio_sample_t *in, *out;

	in = jack_port_get_buffer(input_port, nframes);
	out = jack_port_get_buffer(output_port, nframes);

	for(unsigned int i=0; i<nframes; i++){
		mixedSignal[i] = in[i] + buffer[bufferIndex + i];
	}
	
	memcpy(out, &mixedSignal,
		   sizeof(jack_default_audio_sample_t) * nframes);

	for(unsigned int i=0; i<nframes; i++){
		buffer[bufferIndex + i] = in[i] + mixedSignal[i]/feedBackDelay;
	}

	if(bufferIndex >= sizeof(jack_default_audio_sample_t) * nframes * echoDelay)bufferIndex = 0;
	else bufferIndex += sizeof(jack_default_audio_sample_t) * nframes;

	return 0;
}

/**
 * JACK calls this shutdown_callback if the server ever shuts down or
 * decides to disconnect the client.
 */
void jack_shutdown(void *arg)
{
	exit(1);
}

int main(int argc, char *argv[])
{
	const char **ports;
	const char *client_name = "simple";
	const char *server_name = NULL;
	jack_options_t options = JackNullOption;
	jack_status_t status;

	/* open a client connection to the JACK server */

	client = jack_client_open(client_name, options, &status, server_name);
	if (client == NULL)
	{
		fprintf(stderr, "jack_client_open() failed, "
						"status = 0x%2.0x\n",
				status);
		if (status & JackServerFailed)
		{
			fprintf(stderr, "Unable to connect to JACK server\n");
		}
		exit(1);
	}
	if (status & JackServerStarted)
	{
		fprintf(stderr, "JACK server started\n");
	}
	if (status & JackNameNotUnique)
	{
		client_name = jack_get_client_name(client);
		fprintf(stderr, "unique name `%s' assigned\n", client_name);
	}

	/* tell the JACK server to call `process()' whenever
	   there is work to be done.
	*/

	jack_set_process_callback(client, process, 0);

	/* tell the JACK server to call `jack_shutdown()' if
	   it ever shuts down, either entirely, or if it
	   just decides to stop calling us.
	*/

	jack_on_shutdown(client, jack_shutdown, 0);

	/* display the current sample rate.
	 */

	printf("engine sample rate: %" PRIu32 "\n",
		   jack_get_sample_rate(client));

	/* create two ports */

	input_port = jack_port_register(client, "input",
									JACK_DEFAULT_AUDIO_TYPE,
									JackPortIsInput, 0);
	output_port = jack_port_register(client, "output",
									 JACK_DEFAULT_AUDIO_TYPE,
									 JackPortIsOutput, 0);

	if ((input_port == NULL) || (output_port == NULL))
	{
		fprintf(stderr, "no more JACK ports available\n");
		exit(1);
	}

	/* Tell the JACK server that we are ready to roll.  Our
	 * process() callback will start running now. */

	if (jack_activate(client))
	{
		fprintf(stderr, "cannot activate client");
		exit(1);
	}

	/* Connect the ports.  You can't do this before the client is
	 * activated, because we can't make connections to clients
	 * that aren't running.  Note the confusing (but necessary)
	 * orientation of the driver backend ports: playback ports are
	 * "input" to the backend, and capture ports are "output" from
	 * it.
	 */

	ports = jack_get_ports(client, NULL, NULL,
						   JackPortIsPhysical | JackPortIsOutput);
	if (ports == NULL)
	{
		fprintf(stderr, "no physical capture ports\n");
		exit(1);
	}

	if (jack_connect(client, ports[0], jack_port_name(input_port)))
	{
		fprintf(stderr, "cannot connect input ports\n");
	}

	free(ports);

	ports = jack_get_ports(client, NULL, NULL,
						   JackPortIsPhysical | JackPortIsInput);
	if (ports == NULL)
	{
		fprintf(stderr, "no physical playback ports\n");
		exit(1);
	}

	if (jack_connect(client, jack_port_name(output_port), ports[0]))
	{
		fprintf(stderr, "cannot connect output ports\n");
	}

	free(ports);

	/* keep running until stopped by the user */

	sleep(-1);

	/* this is never reached but if the program
	   had some other way to exit besides being killed,
	   they would be important to call.
	*/

	jack_client_close(client);
	exit(0);
}

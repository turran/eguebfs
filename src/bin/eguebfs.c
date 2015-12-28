#define _GNU_SOURCE

#include <stdio.h>
#include <getopt.h>

#include <Eguebfs.h>
#include <Ecore.h>
#include <Efl_Egueb.h>

static void help(void)
{
	printf("Usage: eguebfs [OPTIONS] FILE MOUNTPOINT\n");
	printf("Where OPTIONS can be one of the following:\n");
	printf("-h Print this screen\n");
	printf("-v Create a window to visualize the file\n");
}

static void window_close_cb(Egueb_Dom_Event *e,
		void *data)
{
	ecore_main_loop_quit();
}

int main(int argc, char **argv)
{
	Eguebfs *efs;
	Egueb_Dom_Node *doc = NULL;
	Egueb_Dom_Window *w = NULL;
	Enesim_Stream *stream;
	Eina_Bool visualize = EINA_FALSE;
	char *short_options = "hv";
	struct option long_options[] = {
		{ "help", 1, 0, 'h' },
		{ "visualize", 1, 0, 'w' },
	};
	int option;
	int ret;

	/* parse the options */
	while ((ret = getopt_long(argc, argv, short_options, long_options,
			&option)) != -1)
	{
		switch (ret)
		{
			case 'h':
			help();
			return 0;

			case 'v':
			visualize = EINA_TRUE;

			default:
			break;
		}
	}

	/* check that we have two more arguments */
	if (argc - optind != 2)
	{
		help();
		return 0;
	}

	ecore_init();
	eguebfs_init();
	efl_egueb_init();

	stream = enesim_stream_file_new(argv[optind], "r");
	if (!stream)
	{
		printf("Fail to load file %s\n", argv[optind]);
		goto shutdown;
	}
	/* create the document */
	if (!egueb_dom_parser_parse(stream, &doc))
	{
		printf("Fail to parse file %s\n", argv[optind]);
		goto shutdown;
	}

	if (visualize)
	{
		Egueb_Dom_Event_Target *et;

		/* create the window */
		w = efl_egueb_window_auto_new(egueb_dom_node_ref(doc), 0, 0, -1, -1);
		if (!w)
		{
			printf("Fail to create window\n");
			goto no_window;
		}
		et = EGUEB_DOM_EVENT_TARGET(w);
		egueb_dom_event_target_event_listener_add(et,
				EGUEB_DOM_EVENT_WINDOW_CLOSE,
				window_close_cb,
				EINA_TRUE, NULL);
	}
	/* mount and wait */
	efs = eguebfs_mount(egueb_dom_node_ref(doc), argv[optind + 1]);
	ecore_main_loop_begin();
	eguebfs_umount(efs);
	if (w)
		egueb_dom_window_unref(w);

no_window:
	egueb_dom_node_unref(doc);
shutdown:
	eguebfs_shutdown();
	efl_egueb_shutdown();
	ecore_shutdown();
	return 0;
}

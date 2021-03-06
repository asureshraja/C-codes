#include <ev.h>
#include <stdio.h>
ev_io stdin_watcher;
ev_timer timeout_watcher;

static void stdin_cb (EV_P_ struct ev_io *w, int revents)
{
	puts ("stdin ready");

	ev_io_stop (EV_A_ w);

	ev_unloop (EV_A_ EVUNLOOP_ALL);
}

static void timeout_cb (EV_P_ struct ev_timer *w, int revents)
{
	puts ("timeout");

	ev_unloop (EV_A_ EVUNLOOP_ONE);
}

int main (void)
{

	struct ev_loop *loop = ev_default_loop (0);
	
	ev_io_init (&stdin_watcher, stdin_cb,0, EV_READ);
	ev_io_start (loop, &stdin_watcher);

	ev_timer_init (&timeout_watcher, timeout_cb, 5.5, 0.);
	ev_timer_start (loop, &timeout_watcher);

	ev_loop (loop, 0);

	return 0;
}

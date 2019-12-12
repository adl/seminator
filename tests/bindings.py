import spot
import spot.seminator as sem

aut = spot.translate('G(a | (b U (Gc | Gd)))')
res = sem.seminator(aut).run()

assert aut.num_states() == 4
assert res.num_states() == 5
assert aut.equivalent_to(res)

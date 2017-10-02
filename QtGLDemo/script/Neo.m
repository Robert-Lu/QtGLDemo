down_dir = - [zeros(num_verts, 2), ones(num_verts, 1)];

Gravity = grav_acc * Mass * down_dir;
Pressure = area_press * Area * N;

Near = lt(Distance, 0.01);
DisForce = - 0.1 * Area * Distance_2_rev * grad_E_Dis .* [Near, Near, Near];
% Adhere = gt(Distance_2_rev, 5000);
% AdhereForce = Area * Adhere * Distance_2_rev * grad_E_Dis;
% F = Gravity + Pressure + DisForce;
F = DisForce + Pressure;

LHS = [w_L * Lap; (w_P + w_F) * Mass];
RHS = [zeros(num_verts, 3); w_P * Mass * V + w_F * (Mass * V + F)];

V_prime = LHS \ RHS;

% Upper = gt(V(:, 3), 0.2);
% Bottom = 1 - Upper;
% V_prime = V_prime .* [Upper, Upper, Upper] + V .* [Bottom, Bottom, Bottom];

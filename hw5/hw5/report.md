# Project Report
# Members
 Meihong Ge mg476 / Wenxin Xu wx65
## 1 Sequantial Portion
The overall flow of the algorithm follows the flow chart shown below. 
![Algorithm Flow Chart](./algorithm_flow.png)

Most processes are quite straight forward, but there are two key steps need some attention. First one being the ***Compute the Flow Pattern***. Since the flow of water in our scenario only take the height of point on the grid, we get to precompute the flow pattern before the simulation, so we don’t have to do it during each iteration. 

Each point has a `vector` that contains the coordinates of neighbors that water will flow to given there is enough of them. The coordinate is represented in `pair<int, int>` while the vector itself function like a `monotone stack`. That is, if a neighbor has a height lower than the top element (then its lower than all the element in the stack), the stack is cleared, and this neighbor’s coordinate is pushed into the stack. Neighbor with same height is added without pop and neighbor with larger height is ignored.

Another important thing is ***Add the Leak with Current Water Level***. When running the simulation, an important catch is that water flowing from one point to other points only happens when all the absorption step is done on each spot on the grid. If such rule is not adhered to, error will occur that might result in infinite loop. To prevent this, a temporary `matrix (2-D vector)` is created to store the amount of water that is about to flow into a certain spot at each time step. The actual change to water level only happens when this time step ends.

## 2 Parallel Portion
### 2.1
